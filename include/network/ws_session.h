#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <nlohmann/json.hpp>
#include <memory>
#include <queue>
#include <string>

class MatchLobbyService;
class Player;

class WsSession : public std::enable_shared_from_this<WsSession> {
public:
    WsSession(boost::asio::ip::tcp::socket&& socket, MatchLobbyService& lobby);

    void run();
    void send_json(const nlohmann::json& j);

private:
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes);
    void on_close();

    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    boost::beast::flat_buffer buffer_;
    MatchLobbyService& lobby_;
    std::shared_ptr<Player> player_;
    std::queue<std::string> write_queue_;
    bool closed_{false};
};
