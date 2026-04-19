#pragma once
#include <string>
#include <memory>
#include <functional>
#include <nlohmann/json.hpp>

class Player : public std::enable_shared_from_this<Player> {
public:
    using SendCallback = std::function<void(const nlohmann::json&)>;

    Player(SendCallback send_cb) : send_cb_(std::move(send_cb)) {}

    void send_json(const nlohmann::json& msg) {
        if (send_cb_) {
            send_cb_(msg);
        }
    }

    std::string name = "Player";
    std::string current_room_id = "";
    bool can_chat = true;
    bool can_create_room = true;

    // Server-side move counter (reset each game, validated on win claim)
    int server_move_count = 0;

    void reset_game_move_count() { server_move_count = 0; }

    void attach_opponent(std::shared_ptr<Player> other) {
        opponent_ = other;
    }

    void clear_opponent() {
        opponent_.reset();
    }

    std::shared_ptr<Player> opponent_lock() const {
        return opponent_.lock();
    }

private:
    std::weak_ptr<Player> opponent_;
    SendCallback send_cb_;
};
