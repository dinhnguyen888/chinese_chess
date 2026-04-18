#pragma once
#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
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
    void do_accept();
    void on_accept_request(boost::beast::error_code ec, std::size_t bytes);
    void on_accept(boost::beast::error_code ec);
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes);
    void do_write();
    void on_write(boost::beast::error_code ec, std::size_t bytes);
    void on_close();
    void send_http_error(boost::beast::http::status status, const std::string& message);
    void close_socket();

    boost::beast::websocket::stream<boost::asio::ip::tcp::socket> ws_;
    boost::beast::flat_buffer buffer_;
    boost::beast::http::request<boost::beast::http::string_body> req_;
    MatchLobbyService& lobby_;
    std::shared_ptr<Player> player_;
    std::queue<std::string> write_queue_;
    std::string pending_username_;
    bool closed_{false};
    boost::beast::http::response<boost::beast::http::string_body> error_res_;
};
