#pragma once
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <memory>

class HttpListener : public std::enable_shared_from_this<HttpListener> {
public:
    HttpListener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint);
    void run();

private:
    void do_accept();
    void on_accept(boost::system::error_code ec, boost::asio::ip::tcp::socket socket);

    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
};
