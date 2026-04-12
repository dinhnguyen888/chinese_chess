#include "handlers/admin_handler.h"
#include "service/user_service.h"
#include "service/game_service.h"
#include "service/report_service.h"
#include "utils/jwt_utils.h"
#include "service/match_lobby_service.h"
#include <nlohmann/json.hpp>
#include <boost/beast/http.hpp>
#include <regex>

namespace beast = boost::beast;
namespace http = beast::http;
using json = nlohmann::json;

namespace handlers {

static http::response<http::string_body> make_json_response(http::status status, const json& body, int version, bool keep_alive) {
    http::response<http::string_body> res{status, version};
    res.set(http::field::content_type, "application/json");
    res.keep_alive(keep_alive);
    res.body() = body.dump();
    res.prepare_payload();
    return res;
}

http::response<http::string_body> handle_admin_request(const http::request<http::string_body>& req) {
    auto version = req.version();
    auto keep_alive = req.keep_alive();

    // Verification
    std::string auth_header = req[http::field::authorization];
    if (auth_header.size() < 7 || auth_header.substr(0, 7) != "Bearer ") {
        return make_json_response(http::status::unauthorized, json{{"error", "missing_token"}}, version, keep_alive);
    }
    std::string token = auth_header.substr(7);
    std::string username = utils::jwt::decode_and_verify(token);
    std::string role = utils::jwt::extract_role(token);

    if (username.empty() || role != "admin") {
        return make_json_response(http::status::forbidden, json{{"error", "forbidden"}}, version, keep_alive);
    }

    std::string target = std::string(req.target());

    // GET /admin/users
    if (req.method() == http::verb::get && target.find("/admin/users") == 0) {
        int page = 1;
        int limit = 20;
        
        std::regex page_regex("page=([0-9]+)");
        std::regex limit_regex("limit=([0-9]+)");
        std::smatch match;
        if (std::regex_search(target, match, page_regex)) page = std::stoi(match[1]);
        if (std::regex_search(target, match, limit_regex)) limit = std::stoi(match[1]);

        auto [users, total] = UserService::get_all_users(page, limit);
        json j_users = json::array();
        for (auto const& u : users) {
            j_users.push_back({
                {"id", u.id}, 
                {"username", u.username}, 
                {"role", u.role},
                {"banned_until", u.banned_until},
                {"can_chat", u.can_chat},
                {"can_create_room", u.can_create_room}
            });
        }
        return make_json_response(http::status::ok, json{{"data", j_users}, {"total", total}}, version, keep_alive);
    }

    // POST /admin/users
    if (req.method() == http::verb::post && target == "/admin/users") {
        try {
            auto body = json::parse(req.body());
            if (UserService::create_user(body["username"], body["password"], body["role"])) {
                return make_json_response(http::status::created, json{{"success", true}}, version, keep_alive);
            }
        } catch (...) {}
        return make_json_response(http::status::bad_request, json{{"error", "invalid_data"}}, version, keep_alive);
    }

    // PUT /admin/users
    if (req.method() == http::verb::put && target == "/admin/users") {
        try {
            auto body = json::parse(req.body());
            if (UserService::update_user(body["id"], body["username"], body.value("password", ""), body["role"])) {
                return make_json_response(http::status::ok, json{{"success", true}}, version, keep_alive);
            }
        } catch (...) {}
        return make_json_response(http::status::bad_request, json{{"error", "invalid_data"}}, version, keep_alive);
    }

    // DELETE /admin/users?id=X
    if (req.method() == http::verb::delete_ && target.find("/admin/users") == 0) {
        std::regex id_regex("id=([0-9]+)");
        std::smatch match;
        if (std::regex_search(target, match, id_regex)) {
            int id = std::stoi(match[1]);
            if (UserService::delete_user(id)) {
                return make_json_response(http::status::ok, json{{"success", true}}, version, keep_alive);
            }
        }
        return make_json_response(http::status::bad_request, json{{"error", "invalid_id"}}, version, keep_alive);
    }

    // GET /admin/users/history?username=X
    if (req.method() == http::verb::get && target.find("/admin/users/history") == 0) {
        std::regex user_regex("username=([^&]+)");
        std::smatch match;
        if (std::regex_search(target, match, user_regex)) {
            std::string target_user = match[1];
            auto history = GameService::get_history(target_user, 50);
            json j_history = json::array();
            for (auto const& m : history) {
                j_history.push_back({
                    {"id", m.id},
                    {"opponent", m.opponent},
                    {"result", m.result},
                    {"played_at", m.played_at},
                    {"duration_seconds", m.duration_seconds}
                });
            }
            return make_json_response(http::status::ok, j_history, version, keep_alive);
        }
    }

    // GET /admin/reports
    if (req.method() == http::verb::get && target.find("/admin/reports") == 0) {
        int page = 1;
        int limit = 20;
        
        std::regex page_regex("page=([0-9]+)");
        std::regex limit_regex("limit=([0-9]+)");
        std::smatch match;
        if (std::regex_search(target, match, page_regex)) page = std::stoi(match[1]);
        if (std::regex_search(target, match, limit_regex)) limit = std::stoi(match[1]);

        auto [reports, total] = ReportService::get_all_reports(page, limit);
        json j_reports = json::array();
        for (auto const& r : reports) {
            j_reports.push_back({
                {"id", r.id}, {"reporter", r.reporter}, {"reported", r.reported},
                {"match_id", r.match_id}, {"reason", r.reason}, {"status", r.status}, {"created_at", r.created_at}
            });
        }
        return make_json_response(http::status::ok, json{{"data", j_reports}, {"total", total}}, version, keep_alive);
    }

    // PUT /admin/reports (update status)
    if (req.method() == http::verb::put && target == "/admin/reports") {
        try {
            auto body = json::parse(req.body());
            if (ReportService::update_report_status(body["id"], body["status"])) {
                return make_json_response(http::status::ok, json{{"success", true}}, version, keep_alive);
            }
        } catch (...) {}
        return make_json_response(http::status::bad_request, json{{"error", "invalid_data"}}, version, keep_alive);
    }

    // POST /admin/punish
    if (req.method() == http::verb::post && target == "/admin/punish") {
        try {
            auto body = json::parse(req.body());
            std::string target_user = body.value("username", "");
            int ban_days = body.value("ban_days", 0);
            bool can_chat = body.value("can_chat", true);
            bool can_create_room = body.value("can_create_room", true);

            if (UserService::apply_punishment(target_user, ban_days, can_chat, can_create_room)) {
                if (g_lobby) {
                    std::string reason = body.value("reason", "Vi phạm tiêu chuẩn cộng đồng");
                    std::string reporter = body.value("reporter", "Hệ thống");
                    g_lobby->notify_punishment(target_user, reason, reporter);
                }
                return make_json_response(http::status::ok, json{{"success", true}}, version, keep_alive);
            }
        } catch (...) {}
        return make_json_response(http::status::bad_request, json{{"error", "invalid_data"}}, version, keep_alive);
    }

    return make_json_response(http::status::not_found, json{{"error", "not_found"}}, version, keep_alive);
}

}
