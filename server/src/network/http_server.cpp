#include "network/http_server.h"
#include "network/http_session.h"
#include <iostream>

namespace beast = boost::beast;
using tcp = boost::asio::ip::tcp;

static void fail(boost::system::error_code ec, char const* what) {
    std::cerr << what << ": " << ec.message() << "\n";
}

HttpListener::HttpListener(boost::asio::io_context& ioc, tcp::endpoint endpoint)
    : ioc_(ioc), acceptor_(ioc) {
    boost::system::error_code ec;
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

void HttpListener::run() {
    do_accept();
}

void HttpListener::do_accept() {
    auto self = shared_from_this();
    acceptor_.async_accept([self](boost::system::error_code ec, tcp::socket socket) {
        self->on_accept(ec, std::move(socket));
    });
}

void HttpListener::on_accept(boost::system::error_code ec, tcp::socket socket) {
    if (ec) {
        fail(ec, "accept");
    } else {
        std::make_shared<HttpSession>(std::move(socket))->run();
    }
    do_accept();
}
