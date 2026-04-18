#include "network/ws_listener.h"
#include "network/ws_session.h"
#include "service/match_lobby_service.h"
#include <iostream>

namespace beast = boost::beast;
using boost::asio::ip::tcp;

static void fail(beast::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

Listener::Listener(boost::asio::io_context& ioc, tcp::endpoint endpoint, std::shared_ptr<MatchLobbyService> lobby)
    : ioc_(ioc), acceptor_(ioc), lobby_(std::move(lobby)) {
    beast::error_code ec;
    acceptor_.open(endpoint.protocol(), ec);
    if (ec) {
        fail(ec, "open");
        return;
    }
    acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
    if (ec) {
        fail(ec, "set_option");
        return;
    }
    acceptor_.bind(endpoint, ec);
    if (ec) {
        fail(ec, "bind");
        return;
    }
    acceptor_.listen(boost::asio::socket_base::max_listen_connections, ec);
    if (ec) {
        fail(ec, "listen");
        return;
    }
}

void Listener::run() {
    do_accept();
}

void Listener::do_accept() {
    auto self = shared_from_this();
    acceptor_.async_accept([self](beast::error_code ec, tcp::socket socket) {
        self->on_accept(ec, std::move(socket));
    });
}

void Listener::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        fail(ec, "accept");
    } else {
        std::make_shared<WsSession>(std::move(socket), *lobby_)->run();
    }
    do_accept();
}
