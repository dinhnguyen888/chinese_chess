#include "network/http_session.h"
#include "service/user_service.h"
#include <chrono>
#include "utils/jwt_utils.h"
#include <nlohmann/json.hpp>
#include "service/game_service.h"
#include "service/report_service.h"
#include "handlers/admin_handler.h"
#include <regex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;
using json = nlohmann::json;

static http::response<http::string_body> make_json_response(http::status status, const json& body, int version, bool keep_alive) {
    http::response<http::string_body> res{status, version};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(keep_alive);
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

static bool parse_body(const http::request<http::string_body>& req, json& out) {
    try {
        out = json::parse(req.body());
        return true;
    } catch (...) {
        return false;
    }
}

static http::response<http::string_body> handle_register(const http::request<http::string_body>& req) {
    json body;
    if (!parse_body(req, body)) {
        return make_json_response(http::status::bad_request, json{{"type", "error"}, {"message", "invalid_json"}}, req.version(), req.keep_alive());
    }

    std::string username = body.value("username", "");
    std::string password = body.value("password", "");
    if (username.empty() || password.empty()) {
        return make_json_response(http::status::bad_request, json{{"type", "error"}, {"message", "missing_fields"}}, req.version(), req.keep_alive());
    }

    if (!UserService::register_user(username, password)) {
        return make_json_response(http::status::conflict, json{{"type", "error"}, {"message", "Tên đăng nhập đã tồn tại hoặc có lỗi xảy ra"}}, req.version(), req.keep_alive());
    }

    std::string token = utils::jwt::generate_token(username, "user");
    return make_json_response(http::status::ok, json{{"type", "auth_success"}, {"username", username}, {"action", "register"}, {"token", token}}, req.version(), req.keep_alive());
}

static http::response<http::string_body> handle_login(const http::request<http::string_body>& req) {
    json body;
    if (!parse_body(req, body)) {
        return make_json_response(http::status::bad_request, json{{"type", "error"}, {"message", "invalid_json"}}, req.version(), req.keep_alive());
    }

    std::string username = body.value("username", "");
    std::string password = body.value("password", "");
    if (username.empty() || password.empty()) {
        return make_json_response(http::status::bad_request, json{{"type", "error"}, {"message", "missing_fields"}}, req.version(), req.keep_alive());
    }

    auto user_opt = UserService::login_user(username, password);
    if (!user_opt) {
        return make_json_response(http::status::unauthorized, json{{"type", "error"}, {"message", "Sai tên đăng nhập hoặc mật khẩu"}}, req.version(), req.keep_alive());
    }

    if (!user_opt->banned_until.empty()) {
        return make_json_response(http::status::forbidden, json{{"type", "error"}, {"message", "Tài khoản bị khóa đến " + user_opt->banned_until}}, req.version(), req.keep_alive());
    }

    std::string token = utils::jwt::generate_token(user_opt->username, user_opt->role);
    return make_json_response(http::status::ok, 
        json{{"type", "auth_success"}, {"username", user_opt->username}, {"action", "login"}, {"token", token}, 
             {"can_chat", user_opt->can_chat}, {"can_create_room", user_opt->can_create_room}}, 
        req.version(), req.keep_alive());
}


static http::response<http::string_body> handle_report(const http::request<http::string_body>& req) {
    json body;
    if (!parse_body(req, body)) return make_json_response(http::status::bad_request, json{{"error", "invalid_json"}}, req.version(), req.keep_alive());

    std::string token = body.value("token", "");
    std::string reporter = utils::jwt::decode_and_verify(token);
    if (reporter.empty()) return make_json_response(http::status::unauthorized, json{{"error", "unauthorized"}}, req.version(), req.keep_alive());

    std::string reported = body.value("reported", "");
    std::string reason = body.value("reason", "");
    int match_id = body.value("match_id", 0);

    if (reported.empty() || reason.empty()) {
        return make_json_response(http::status::bad_request, json{{"error", "missing_fields"}}, req.version(), req.keep_alive());
    }

    if (ReportService::create_report(reporter, reported, match_id, reason)) {
        return make_json_response(http::status::ok, json{{"success", true}}, req.version(), req.keep_alive());
    }
    return make_json_response(http::status::internal_server_error, json{{"error", "db_error"}}, req.version(), req.keep_alive());
}

static http::response<http::string_body> handle_request(const http::request<http::string_body>& req) {
    std::string target = std::string(req.target());

    if (req.method() == http::verb::post && target == "/report") {
        return handle_report(req);
    }
    if (req.target() == "/register") {
        return handle_register(req);
    }
    if (target == "/login") {
        return handle_login(req);
    }
    if (target.find("/admin") == 0) {
        return handlers::handle_admin_request(req);
    }

    return make_json_response(http::status::not_found, json{{"type", "error"}, {"message", "not_found"}}, req.version(), req.keep_alive());
}

HttpSession::HttpSession(tcp::socket&& socket)
    : stream_(std::move(socket)) {}

void HttpSession::run() {
    stream_.expires_after(std::chrono::seconds(30));
    do_read();
}

void HttpSession::do_read() {
    req_ = {};
    auto self = shared_from_this();
    http::async_read(stream_, buffer_, req_, [self](beast::error_code ec, std::size_t bytes) {
        self->on_read(ec, bytes);
    });
}

void HttpSession::on_read(beast::error_code ec, std::size_t) {
    if (ec == http::error::end_of_stream)
        return do_close();
    if (ec)
        return;

    res_ = handle_request(req_);
    bool close = res_.need_eof();
    auto self = shared_from_this();
    http::async_write(stream_, res_, [self, close](beast::error_code write_ec, std::size_t bytes) {
        self->on_write(close, write_ec, bytes);
    });
}

void HttpSession::on_write(bool close, beast::error_code ec, std::size_t) {
    if (ec)
        return;
    if (close)
        return do_close();
    do_read();
}

void HttpSession::do_close() {
    beast::error_code ec;
    stream_.socket().shutdown(tcp::socket::shutdown_both, ec);
}
