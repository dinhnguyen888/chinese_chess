#include "network/ws_session.h"
#include "service/match_lobby_service.h"
#include "handlers/packet_dispatcher.h"
#include "type/player.h"
#include "utils/jwt_utils.h"
#include "service/user_service.h"
#include <iostream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using boost::asio::ip::tcp;
using json = nlohmann::json;

static void fail(beast::error_code ec, char const* what) {
    if (ec != websocket::error::closed)
        std::cerr << what << ": " << ec.message() << "\n";
}

static std::string extract_query_param(const std::string& target, const std::string& key) {
    auto qpos = target.find('?');
    if (qpos == std::string::npos)
        return "";
    std::string query = target.substr(qpos + 1);
    size_t start = 0;
    while (start < query.size()) {
        auto end = query.find('&', start);
        if (end == std::string::npos)
            end = query.size();
        auto eq = query.find('=', start);
        if (eq != std::string::npos && eq < end) {
            std::string k = query.substr(start, eq - start);
            if (k == key) {
                return query.substr(eq + 1, end - eq - 1);
            }
        }
        start = end + 1;
    }
    return "";
}

WsSession::WsSession(tcp::socket&& socket, MatchLobbyService& lobby)
    : ws_(std::move(socket)), lobby_(lobby) {}

void WsSession::run() {
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(beast::http::field::server, "chinese-chess-ws");
    }));

    std::weak_ptr<WsSession> weak_self = shared_from_this();
    player_ = std::make_shared<Player>([weak_self](const json& msg) {
        if (auto self = weak_self.lock())
            self->send_json(msg);
    });

    do_accept();
}

void WsSession::send_json(const json& j) {
    std::string text = j.dump();
    bool idle = write_queue_.empty();
    write_queue_.push(std::move(text));
    if (idle)
        do_write();
}

void WsSession::do_accept() {
    auto self = shared_from_this();
    http::async_read(ws_.next_layer(), buffer_, req_,
        [self](beast::error_code ec, std::size_t bytes) { self->on_accept_request(ec, bytes); });
}

void WsSession::on_accept_request(beast::error_code ec, std::size_t) {
    if (ec)
        return fail(ec, "read_handshake");

    if (!websocket::is_upgrade(req_)) {
        send_http_error(http::status::bad_request, "expected_websocket_upgrade");
        return;
    }

    std::string token = extract_query_param(std::string(req_.target()), "token");
    std::string username = utils::jwt::decode_and_verify(token);
    if (username.empty()) {
        send_http_error(http::status::unauthorized, "invalid_or_expired_token");
        return;
    }
    pending_username_ = std::move(username);

    auto self = shared_from_this();
    ws_.async_accept(req_, [self](beast::error_code ec) { self->on_accept(ec); });
}

void WsSession::on_accept(beast::error_code ec) {
    if (ec)
        return fail(ec, "accept");

    if (!pending_username_.empty() && player_) {
        player_->name = pending_username_;
        
        auto user_opt = UserService::get_user_by_username(player_->name);
        if (user_opt) {
            player_->can_chat = user_opt->can_chat;
            player_->can_create_room = user_opt->can_create_room;
        }

        // Đăng ký player vào danh sách online để Admin có thể bắn notify
        lobby_.register_player(player_->name, player_);

        player_->send_json(json{
            {"type", "auth_success"}, 
            {"username", player_->name}, 
            {"action", "verify"},
            {"can_chat", player_->can_chat},
            {"can_create_room", player_->can_create_room}
        });
    }

    do_read();
}

void WsSession::do_read() {
    auto self = shared_from_this();
    ws_.async_read(buffer_, [self](beast::error_code ec, std::size_t n) { self->on_read(ec, n); });
}

void WsSession::on_read(beast::error_code ec, std::size_t) {
    if (ec) {
        if (ec != websocket::error::closed)
            fail(ec, "read");
        on_close();
        return;
    }

    std::string text = beast::buffers_to_string(buffer_.data());
    buffer_.consume(buffer_.size());
    PacketDispatcher::instance().dispatch(player_, lobby_, text);
    do_read();
}

void WsSession::do_write() {
    if (write_queue_.empty())
        return;
    auto self = shared_from_this();
    ws_.async_write(boost::asio::buffer(write_queue_.front()),
        [self](beast::error_code ec, std::size_t n) { self->on_write(ec, n); });
}

void WsSession::on_write(beast::error_code ec, std::size_t) {
    if (ec) {
        fail(ec, "write");
        on_close();
        return;
    }
    write_queue_.pop();
    if (!write_queue_.empty())
        do_write();
}

void WsSession::send_http_error(http::status status, const std::string& message) {
    error_res_ = {status, req_.version()};
    error_res_.set(http::field::content_type, "application/json");
    error_res_.body() = json{{"type", "error"}, {"message", message}}.dump();
    error_res_.prepare_payload();

    auto self = shared_from_this();
    http::async_write(ws_.next_layer(), error_res_, [self](beast::error_code, std::size_t) {
        self->close_socket();
    });
}

void WsSession::close_socket() {
    beast::error_code ec;
    ws_.next_layer().shutdown(tcp::socket::shutdown_both, ec);
    ws_.next_layer().close(ec);
}

void WsSession::on_close() {
    if (closed_)
        return;
    closed_ = true;

    if (player_) {
        lobby_.unregister_player(player_->name);
        lobby_.cancel_waiting(player_);
        if (auto peer = player_->opponent_lock()) {
            peer->clear_opponent();
            peer->send_json(json{{"type", "opponent_left"}});
        }
        player_->clear_opponent();
    }
}
