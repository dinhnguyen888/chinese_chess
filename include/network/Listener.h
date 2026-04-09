#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <memory>

class MatchLobbyService;

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(boost::asio::io_context& ioc, boost::asio::ip::tcp::endpoint endpoint, std::shared_ptr<MatchLobbyService> lobby);
    void run();

private:
    void do_accept();
    void on_accept(boost::beast::error_code ec, boost::asio::ip::tcp::socket socket);

    boost::asio::io_context& ioc_;
    boost::asio::ip::tcp::acceptor acceptor_;
    std::shared_ptr<MatchLobbyService> lobby_;
};
