#include "service/anti_cheat_service.h"
#include "type/player.h"
#include <iostream>
#include <sstream>

using json = nlohmann::json;
using steady_clk = std::chrono::steady_clock;

// ---------------------------------------------------------------------------
// Singleton
// ---------------------------------------------------------------------------
AntiCheatService& AntiCheatService::instance() {
    static AntiCheatService inst;
    return inst;
}

// ---------------------------------------------------------------------------
// Internal helpers
// ---------------------------------------------------------------------------
PlayerAntiCheatState& AntiCheatService::get_state(const std::string& name) {
    // Caller must hold mutex_
    return states_[name];
}

static void log_violation(const std::string& player, const std::string& reason) {
    std::cerr << "[AntiCheat] VIOLATION | player=" << player << " | " << reason << "\n";
}

// ---------------------------------------------------------------------------
// on_connect / on_disconnect
// ---------------------------------------------------------------------------
void AntiCheatService::on_connect(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& s = get_state(player_name);
    s.rapid_reconnect_count++;
    if (s.rapid_reconnect_count > 3) {
        log_violation(player_name, "rapid_reconnect count=" + std::to_string(s.rapid_reconnect_count));
    }
}

void AntiCheatService::on_disconnect(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    // Nothing to do for now — state is preserved across reconnects intentionally
    (void)player_name;
}

void AntiCheatService::reset(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    states_.erase(player_name);
}

// ---------------------------------------------------------------------------
// on_game_start / on_game_end
// ---------------------------------------------------------------------------
void AntiCheatService::on_game_start(const std::string& player_name) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& s = get_state(player_name);
    s.game_start_time = steady_clk::now();
    s.game_active     = true;
}

int AntiCheatService::on_game_end(const std::string& player_name, int duration_seconds, int move_count) {
    std::lock_guard<std::mutex> lock(mutex_);
    auto& s = get_state(player_name);
    s.game_active = false;

    int suspicion = 0;

    // 1. Suspiciously short game
    if (duration_seconds < MIN_SANE_GAME_DURATION && move_count > 0) {
        log_violation(player_name, "game ended suspiciously fast: " + std::to_string(duration_seconds) + "s");
        suspicion += 30;
    }

    // 2. Move speed too high (bot-like)
    if (duration_seconds > 0) {
        double moves_per_min = (move_count * 60.0) / duration_seconds;
        if (moves_per_min > MAX_SANE_MOVES_PER_MIN) {
            log_violation(player_name, "move speed too high: " + std::to_string((int)moves_per_min) + "/min");
            suspicion += 40;
        }
    }

    // 3. Accumulated violations during game
    if (s.violation_count >= MAX_VIOLATIONS_BEFORE_LOG) {
        log_violation(player_name, "high accumulated violation count: " + std::to_string(s.violation_count));
        suspicion += s.violation_count * 5;
    }

    return suspicion;
}

// Move flood: MAX_MOVES_PER_SECOND in 1-second window (>2 = cheating in Chinese Chess)
// ---------------------------------------------------------------------------
bool AntiCheatService::check_move_flood(PlayerAntiCheatState& s, const std::string& player_name) {
    auto now = steady_clk::now();
    auto cutoff = now - std::chrono::seconds(1);

    while (!s.move_timestamps.empty() && s.move_timestamps.front() < cutoff)
        s.move_timestamps.pop_front();

    s.move_timestamps.push_back(now);

    if ((int)s.move_timestamps.size() > MAX_MOVES_PER_SECOND) {
        s.violation_count++;
        log_violation(player_name, "move flood: " + std::to_string(s.move_timestamps.size()) + " moves/s");
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Same-type spam: sending the same message type repeatedly
// ---------------------------------------------------------------------------
bool AntiCheatService::check_same_type_spam(PlayerAntiCheatState& s, const std::string& type, const std::string& player_name) {
    if (type == s.last_type) {
        s.same_type_streak++;
    } else {
        s.same_type_streak = 1;
        s.last_type = type;
    }

    if (s.same_type_streak > MAX_SAME_TYPE_STREAK) {
        s.violation_count++;
        log_violation(player_name, "same-type spam: type=" + type + " x" + std::to_string(s.same_type_streak));
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Payload sanity: validate expected fields per packet type
// ---------------------------------------------------------------------------
bool AntiCheatService::check_payload_sanity(const json& msg, const std::string& type, const std::string& player_name) {
    // move: packet must be a JSON object
    if (type == "move") {
        if (!msg.is_object()) {
            log_violation(player_name, "move packet is not a JSON object");
            return false;
        }
        // NOTE: coordinate range validation is intentionally omitted.
        // The client owns the coordinate format; server-side range checks
        // would require knowing the exact [row,col] vs [col,row] convention
        // and would risk false-positives on valid edge moves (e.g. row 9).
    }

    // chat: message field must be a non-empty string, max 300 chars
    if (type == "chat") {
        if (!msg.contains("message") || !msg["message"].is_string()) {
            log_violation(player_name, "chat packet missing 'message' field");
            return false;
        }
        const std::string& text = msg["message"].get<std::string>();
        if (text.empty() || text.size() > 300) {
            log_violation(player_name, "chat message length invalid: " + std::to_string(text.size()));
            return false;
        }
    }

    // join_room / create_room: room_id / name must be strings
    if (type == "join_room" && msg.contains("roomId")) {
        if (!msg["roomId"].is_string()) {
            log_violation(player_name, "join_room roomId is not a string");
            return false;
        }
    }

    // game_result: result must be "win", "lose" or "draw"
    if (type == "game_result" && msg.contains("result")) {
        if (!msg["result"].is_string()) {
            log_violation(player_name, "game_result.result is not a string");
            return false;
        }
        const std::string r = msg["result"].get<std::string>();
        if (r != "win" && r != "lose" && r != "draw") {
            log_violation(player_name, "game_result.result has invalid value: " + r);
            return false;
        }
    }

    return true;
}

// ---------------------------------------------------------------------------
// Main entry-point called by PacketDispatcher BEFORE routing
// ---------------------------------------------------------------------------
bool AntiCheatService::check_packet(std::shared_ptr<Player> player, const json& msg) {
    const std::string& name = player->name;
    const std::string  type = msg.value("type", "");

    std::lock_guard<std::mutex> lock(mutex_);
    auto& s = get_state(name);

    // 1. Same-type spam guard
    if (!check_same_type_spam(s, type, name)) {
        return false;
    }

    // 2. Move-specific flood (>2 moves/s = impossible in normal Chess)
    if (type == "move") {
        if (!check_move_flood(s, name)) {
            player->send_json(json{{"type", "error"}, {"message", "Move flood detected: slow down"}});
            return false;
        }
    }

    // 3. Payload sanity
    if (!check_payload_sanity(msg, type, name)) {
        player->send_json(json{{"type", "error"}, {"message", "Invalid packet payload"}});
        return false;
    }

    return true;
}
