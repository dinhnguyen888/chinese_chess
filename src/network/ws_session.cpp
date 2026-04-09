#include "network/ws_session.h"
#include "service/match_lobby_service.h"
#include "handlers/packet_dispatcher.h"
#include "type/player.h"
#include <iostream>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
using boost::asio::ip::tcp;
using json = nlohmann::json;

static void fail(beast::error_code ec, char const* what) {
    if (ec != websocket::error::closed)
        std::cerr << what << ": " << ec.message() << "\n";
}

WsSession::WsSession(tcp::socket&& socket, MatchLobbyService& lobby)
    : ws_(std::move(socket)), lobby_(lobby) {}

void WsSession::run() {
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(beast::http::field::server, "chinese-chess-ws");
    }));
    
    auto self = shared_from_this();
    player_ = std::make_shared<Player>([self](const json& msg) {
        self->send_json(msg);
    });

    ws_.async_accept([self](beast::error_code ec) { self->on_accept(ec); });
}

void WsSession::send_json(const json& j) {
    std::string text = j.dump();
    bool idle = write_queue_.empty();
    write_queue_.push(std::move(text));
    if (idle)
        do_write();
}

void WsSession::on_accept(beast::error_code ec) {
    if (ec)
        return fail(ec, "accept");
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

void WsSession::on_close() {
    if (closed_)
        return;
    closed_ = true;
    
    if (player_) {
        lobby_.cancel_waiting(player_);
        if (auto peer = player_->opponent_lock()) {
            peer->clear_opponent();
            peer->send_json(json{{"type", "opponent_left"}});
        }
        player_->clear_opponent();
    }
}
