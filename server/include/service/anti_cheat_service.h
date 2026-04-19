#pragma once
#include <string>
#include <unordered_map>
#include <chrono>
#include <mutex>
#include <memory>
#include <deque>
#include <nlohmann/json.hpp>

class Player;

// Per-player state tracked by the anti-cheat system
struct PlayerAntiCheatState {
    // --- Move flood detection ---
    std::deque<std::chrono::steady_clock::time_point> move_timestamps;

    // --- Suspicious behaviour counters ---
    int violation_count          = 0;
    int invalid_move_count       = 0;
    int rapid_reconnect_count    = 0;

    // --- Game result manipulation ---
    std::chrono::steady_clock::time_point game_start_time;
    bool game_active             = false;

    // --- Last seen packet type (detect type-spam) ---
    std::string last_type;
    int same_type_streak         = 0;
};

class AntiCheatService {
public:
    static AntiCheatService& instance();

    // ---------------------------------------------------------------
    // Called per packet BEFORE dispatching to the actual handler.
    // Returns true if the packet is clean; false if it should be dropped.
    // ---------------------------------------------------------------
    bool check_packet(std::shared_ptr<Player> player, const nlohmann::json& msg);

    // Call when a player starts a game
    void on_game_start(const std::string& player_name);

    // Call when a player ends a game; returns suspicion score (0 = clean)
    int  on_game_end(const std::string& player_name, int duration_seconds, int move_count);

    // Call when player connects / reconnects
    void on_connect(const std::string& player_name);

    // Call when player disconnects
    void on_disconnect(const std::string& player_name);

    // Reset state for a player (e.g. after punishment applied)
    void reset(const std::string& player_name);

private:
    AntiCheatService() = default;

    PlayerAntiCheatState& get_state(const std::string& name);

    bool check_move_flood(PlayerAntiCheatState& s, const std::string& player_name);
    bool check_same_type_spam(PlayerAntiCheatState& s, const std::string& type, const std::string& player_name);
    bool check_payload_sanity(const nlohmann::json& msg, const std::string& type, const std::string& player_name);

    std::unordered_map<std::string, PlayerAntiCheatState> states_;
    std::mutex mutex_;

    // --- Thresholds ---
    static constexpr int  MAX_MOVES_PER_SECOND      = 2;    // >2 moves/s is impossible in normal play (1 move per turn)
    static constexpr int  MAX_SAME_TYPE_STREAK      = 8;    // repeated identical type in a row
    static constexpr int  MAX_VIOLATIONS_BEFORE_LOG = 5;    // log escalation after N violations
    static constexpr int  MIN_SANE_GAME_DURATION    = 5;    // seconds — games shorter than this are sus
    static constexpr int  MAX_SANE_MOVES_PER_MIN    = 120;  // moves per minute ceiling
};
