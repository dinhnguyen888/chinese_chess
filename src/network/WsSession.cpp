#include "network/WsSession.h"
#include "game/MatchLobby.h"
#include "db/Database.h"
#include <iostream>
#include <jwt-cpp/jwt.h>
#include <jwt-cpp/traits/nlohmann-json/defaults.h>

namespace beast = boost::beast;
namespace websocket = beast::websocket;
using boost::asio::ip::tcp;
using json = nlohmann::json;

const std::string JWT_SECRET = "super_secret_key_123";

static std::string generate_jwt(const std::string& username) {
    auto token = jwt::create()
        .set_issuer("chinese_chess")
        .set_type("JWS")
        .set_payload_claim("username", jwt::claim(username))
        .set_expires_at(std::chrono::system_clock::now() + std::chrono::hours(24 * 7))
        .sign(jwt::algorithm::hs256{JWT_SECRET});
    return token;
}

static std::string decode_and_verify_jwt(const std::string& token) {
    try {
        auto decoded = jwt::decode(token);
        auto verifier = jwt::verify()
            .allow_algorithm(jwt::algorithm::hs256{JWT_SECRET})
            .with_issuer("chinese_chess");
        verifier.verify(decoded);
        return decoded.get_payload_claim("username").as_string();
    } catch (const std::exception& e) {
        std::cerr << "JWT Verification Failed: " << e.what() << "\n";
        return "";
    }
}

static void fail(beast::error_code ec, char const* what) {
    if (ec != websocket::error::closed)
        std::cerr << what << ": " << ec.message() << "\n";
}

WsSession::WsSession(tcp::socket&& socket, MatchLobby& lobby)
    : ws_(std::move(socket)), lobby_(lobby) {}

void WsSession::run() {
    ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
    ws_.set_option(websocket::stream_base::decorator([](websocket::response_type& res) {
        res.set(beast::http::field::server, "chinese-chess-ws");
    }));
    auto self = shared_from_this();
    ws_.async_accept([self](beast::error_code ec) { self->on_accept(ec); });
}

void WsSession::send_json(const json& j) {
    std::string text = j.dump();
    bool idle = write_queue_.empty();
    write_queue_.push(std::move(text));
    if (idle)
        do_write();
}

void WsSession::attach_opponent(const std::shared_ptr<WsSession>& other) {
    opponent_ = other;
}

void WsSession::clear_opponent() {
    opponent_.reset();
}

std::shared_ptr<WsSession> WsSession::opponent_lock() const {
    return opponent_.lock();
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

void WsSession::on_read(beast::error_code ec, std::size_t /*bytes*/) {
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

void WsSession::handle_message(const std::string& text) {
    json msg;
    try {
        msg = json::parse(text);
    } catch (const json::parse_error&) {
        send_json(json{{"type", "error"}, {"message", "invalid_json"}});
        return;
    }

    const std::string type = msg.value("type", "");
    if (msg.contains("name")) name_ = msg.value("name", name_.empty() ? "Player" : name_);

    if (type == "verify_jwt") {
        std::string token = msg.value("token", "");
        std::string user = decode_and_verify_jwt(token);
        if (!user.empty()) {
            name_ = user;
            // Token is still good, extend or just return auth_success
            send_json(json{{"type", "auth_success"}, {"username", user}, {"action", "verify"}});
        } else {
            send_json(json{{"type", "auth_fail"}, {"message", "Phiên đăng nhập hết hạn hoặc không hợp lệ"}});
        }
        return;
    }

    if (type == "register") {
        std::string user = msg.value("username", "");
        std::string pass = msg.value("password", "");
        if (Database::get_instance().register_user(user, pass)) {
            name_ = user;
            std::string token = generate_jwt(user);
            send_json(json{{"type", "auth_success"}, {"username", user}, {"action", "register"}, {"token", token}});
        } else {
            send_json(json{{"type", "error"}, {"message", "Tên đăng nhập đã tồn tại hoặc có lỗi xảy ra"}});
        }
        return;
    }
    if (type == "login") {
        std::string user = msg.value("username", "");
        std::string pass = msg.value("password", "");
        if (Database::get_instance().login_user(user, pass)) {
            name_ = user;
            std::string token = generate_jwt(user);
            send_json(json{{"type", "auth_success"}, {"username", user}, {"action", "login"}, {"token", token}});
        } else {
            send_json(json{{"type", "error"}, {"message", "Sai tên đăng nhập hoặc mật khẩu"}});
        }
        return;
    }

    if (type == "find_match") {
        lobby_.try_pair(shared_from_this());
        return;
    }
    if (type == "create_room") {
        bool auto_start = msg.value("autoStart", false);
        lobby_.create_room(shared_from_this(), msg.value("roomName", "My Room"), auto_start);
        return;
    }
    if (type == "join_room") {
        lobby_.join_room(shared_from_this(), msg.value("roomId", ""));
        return;
    }
    if (type == "start_game") {
        lobby_.start_game(shared_from_this(), msg.value("roomId", ""));
        return;
    }
    if (type == "list_rooms") {
        lobby_.get_room_list(shared_from_this());
        return;
    }
    if (type == "search_room") {
        lobby_.search_room(shared_from_this(), msg.value("query", ""));
        return;
    }
    if (type == "chat") {
        lobby_.chat_in_room(shared_from_this(), msg.value("message", ""));
        return;
    }
    if (type == "move") {
        auto peer = opponent_lock();
        if (peer) peer->send_json(msg);

        // ── Tự động lưu lịch sử khi trận kết thúc ──
        // winner: 1 = người gửi move thắng, -1 = đối thủ thắng, null = chưa xong
        if (!msg["winner"].is_null() && peer) {
            int winner_side = msg["winner"].get<int>();
            std::string my_name  = name_;
            std::string opp_name = peer->name_;

            // Tính thời lượng (nếu frontend gửi kèm duration_seconds)
            int duration = msg.value("duration_seconds", 0);

            std::string my_result, opp_result;
            if (winner_side == 1) {          // người gửi move thắng
                my_result  = "win";
                opp_result = "lose";
            } else if (winner_side == -1) {  // đối thủ thắng
                my_result  = "lose";
                opp_result = "win";
            } else {
                my_result = opp_result = "draw";
            }

            // Đặt biến moves_array rỗng (nếu có thì lấy từ frontend)
            std::vector<std::string> moves_array;
            if (msg.contains("moves") && msg["moves"].is_array()) {
                for (const auto& m : msg["moves"]) {
                    moves_array.push_back(m.get<std::string>());
                }
            }

            if (!my_name.empty() && !opp_name.empty()) {
                Database::get_instance().save_match(my_name,  opp_name, my_result,  duration, moves_array);
                Database::get_instance().save_match(opp_name, my_name,  opp_result, duration, moves_array);
                std::cout << "Match saved: " << my_name << "(" << my_result << ") vs "
                          << opp_name << "(" << opp_result << ")\n";
            }
        }
        return;
    }
    if (type == "ping") {
        send_json(json{{"type", "pong"}});
        return;
    }

    if (type == "get_history") {
        std::string user = name_;
        auto records = Database::get_instance().get_history(user, 20);
        json arr = json::array();
        for (const auto& r : records) {
            arr.push_back({
                {"id", r.id},
                {"opponent", r.opponent},
                {"result", r.result},
                {"played_at", r.played_at},
                {"duration_seconds", r.duration_seconds}
            });
        }
        send_json(json{{"type", "history_list"}, {"records", arr}});
        return;
    }
    if (type == "get_replay") {
        int match_id = msg.value("match_id", -1);
        if (match_id != -1) {
            auto moves = Database::get_instance().get_match_moves(match_id);
            json moves_arr = json::array();
            for (const auto& m : moves) {
                moves_arr.push_back(m);
            }
            send_json(json{{"type", "replay_data"}, {"match_id", match_id}, {"moves", moves_arr}});
        }
        return;
    }
    if (type == "game_result") {
        std::string opponent = msg.value("opponent", "");
        std::string result   = msg.value("result", "draw");  // "win" | "lose" | "draw"
        int duration         = msg.value("duration_seconds", 0);
        
        std::vector<std::string> moves_array;
        if (msg.contains("moves") && msg["moves"].is_array()) {
            for (const auto& m : msg["moves"]) {
                moves_array.push_back(m.get<std::string>());
            }
        }

        if (!name_.empty() && !opponent.empty()) {
            Database::get_instance().save_match(name_, opponent, result, duration, moves_array);
            // Cũng lưu phía đối thủ với kết quả ngược lại
            std::string opp_result = (result == "win") ? "lose" : (result == "lose" ? "win" : "draw");
            Database::get_instance().save_match(opponent, name_, opp_result, duration, moves_array);
        }
        return;
    }

    send_json(json{{"type", "error"}, {"message", "unknown_type"}});
}

void WsSession::do_write() {
    if (write_queue_.empty())
        return;
    auto self = shared_from_this();
    ws_.async_write(boost::asio::buffer(write_queue_.front()),
        [self](beast::error_code ec, std::size_t n) { self->on_write(ec, n); });
}

void WsSession::on_write(beast::error_code ec, std::size_t /*bytes*/) {
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
    lobby_.cancel_waiting(shared_from_this());
    
    if (auto peer = opponent_lock()) {
        peer->clear_opponent();
        peer->send_json(json{{"type", "opponent_left"}});
    }
    clear_opponent();
}
