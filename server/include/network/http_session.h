#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <memory>

class HttpSession : public std::enable_shared_from_this<HttpSession> {
public:
    explicit HttpSession(boost::asio::ip::tcp::socket&& socket);
    void run();

private:
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes);
    void on_write(bool close, boost::beast::error_code ec, std::size_t bytes);
    void do_close();

    boost::beast::tcp_stream stream_;
    boost::beast::flat_buffer buffer_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    boost::beast::http::response<boost::beast::http::string_body> res_;
};
