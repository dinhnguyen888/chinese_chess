#include <boost/asio.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/websocket.hpp>
#include <iostream>
#include <memory>
#include <mutex>
#include <nlohmann/json.hpp>
#include <queue>
#include <string>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
using boost::asio::ip::tcp;
using json = nlohmann::json;

void fail(beast::error_code ec, char const* what) {
    if (ec != websocket::error::closed)
        std::cerr << what << ": " << ec.message() << "\n";
}

class WsSession;

class MatchLobby {
public:
    void try_pair(std::shared_ptr<WsSession> session);

    void cancel_waiting(const std::shared_ptr<WsSession>& session) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (waiting_ == session)
            waiting_.reset();
    }

private:
    std::mutex mutex_;
    std::shared_ptr<WsSession> waiting_;
};

class WsSession : public std::enable_shared_from_this<WsSession> {
public:
    WsSession(tcp::socket&& socket, MatchLobby& lobby)
        : ws_(std::move(socket)), lobby_(lobby) {}

    void run() {
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
            res.set(beast::http::field::server, "chinese-chess-ws");
        }));
        auto self = shared_from_this();
        ws_.async_accept([self](beast::error_code ec) { self->on_accept(ec); });
    }

    void send_json(const json& j) {
        std::string text = j.dump();
        bool idle = write_queue_.empty();
        write_queue_.push(std::move(text));
        if (idle)
            do_write();
    }

    void attach_opponent(const std::shared_ptr<WsSession>& other) {
        opponent_ = other;
    }

    void clear_opponent() { opponent_.reset(); }

    std::shared_ptr<WsSession> opponent_lock() const {
        return opponent_.lock();
    }

private:
    void on_accept(beast::error_code ec) {
        if (ec)
            return fail(ec, "accept");
        do_read();
    }

    void do_read() {
        auto self = shared_from_this();
        ws_.async_read(buffer_, [self](beast::error_code ec, std::size_t n) { self->on_read(ec, n); });
    }

    void on_read(beast::error_code ec, std::size_t /*bytes*/) {
        if (ec) {
            if (ec != websocket::error::closed)
                fail(ec, "read");
            on_close();
            return;
        }

        std::string text = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());
        handle_message(text);
        do_read();
    }

    void handle_message(const std::string& text) {
        json msg;
        try {
            msg = json::parse(text);
        } catch (const json::parse_error&) {
            send_json(json{{"type", "error"}, {"message", "invalid_json"}});
            return;
        }

        const std::string type = msg.value("type", "");
        if (type == "find_match") {
            lobby_.try_pair(shared_from_this());
            return;
        }
        if (type == "move") {
            if (auto peer = opponent_lock())
                peer->send_json(msg);
            return;
        }
        if (type == "ping") {
            send_json(json{{"type", "pong"}});
            return;
        }

        send_json(json{{"type", "error"}, {"message", "unknown_type"}});
    }

    void do_write() {
        if (write_queue_.empty())
            return;
        auto self = shared_from_this();
        ws_.async_write(boost::asio::buffer(write_queue_.front()),
            [self](beast::error_code ec, std::size_t n) { self->on_write(ec, n); });
    }

    void on_write(beast::error_code ec, std::size_t /*bytes*/) {
        if (ec) {
            fail(ec, "write");
            on_close();
            return;
        }
        write_queue_.pop();
        if (!write_queue_.empty())
            do_write();
    }

    void on_close() {
        if (closed_)
            return;
        closed_ = true;
        lobby_.cancel_waiting(shared_from_this());
        if (auto peer = opponent_lock()) {
            peer->clear_opponent();
            peer->send_json(json{{"type", "opponent_left"}});
        }
        clear_opponent();
    }

    websocket::stream<tcp::socket> ws_;
    beast::flat_buffer buffer_;
    MatchLobby& lobby_;
    std::weak_ptr<WsSession> opponent_;
    std::queue<std::string> write_queue_;
    bool closed_{false};
};

void MatchLobby::try_pair(std::shared_ptr<WsSession> session) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!waiting_) {
        waiting_ = std::move(session);
        waiting_->send_json(json{{"type", "queue"}, {"message", "waiting_for_opponent"}});
        return;
    }

    auto a = waiting_;
    waiting_.reset();
    auto b = std::move(session);

    a->attach_opponent(b);
    b->attach_opponent(a);

    constexpr int kRedFirst = 1;
    a->send_json(json{{"type", "matched"}, {"color", "r"}, {"orderSide", kRedFirst}});
    b->send_json(json{{"type", "matched"}, {"color", "b"}, {"orderSide", kRedFirst}});
}

class Listener : public std::enable_shared_from_this<Listener> {
public:
    Listener(boost::asio::io_context& ioc, tcp::endpoint endpoint, std::shared_ptr<MatchLobby> lobby)
        : ioc_(ioc), acceptor_(ioc), lobby_(std::move(lobby)) {
        acceptor_.open(endpoint.protocol());
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true));
        acceptor_.bind(endpoint);
        acceptor_.listen(boost::asio::socket_base::max_listen_connections);
    }

    void run() { do_accept(); }

private:
    void do_accept() {
        auto self = shared_from_this();
        acceptor_.async_accept([self](beast::error_code ec, tcp::socket socket) {
            self->on_accept(ec, std::move(socket));
        });
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            fail(ec, "accept");
        } else {
            std::make_shared<WsSession>(std::move(socket), *lobby_)->run();
        }
        do_accept();
    }

    boost::asio::io_context& ioc_;
    tcp::acceptor acceptor_;
    std::shared_ptr<MatchLobby> lobby_;
};

int main() {
    try {
        auto const address = boost::asio::ip::make_address("0.0.0.0");
        unsigned short port = 8080;

        boost::asio::io_context ioc{1};
        auto lobby = std::make_shared<MatchLobby>();
        std::make_shared<Listener>(ioc, tcp::endpoint{address, port}, lobby)->run();

        std::cout << "WebSocket (Boost Beast) listening on ws://0.0.0.0:" << port << "\n";
        ioc.run();
    } catch (const std::exception& e) {
        std::cerr << "Exception: " << e.what() << "\n";
        return 1;
    }
    return 0;
}
