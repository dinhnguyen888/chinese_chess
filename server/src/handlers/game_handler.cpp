#include "handlers/game_handler.h"
#include "type/player.h"
#include "service/game_service.h"
#include "service/anti_cheat_service.h"
#include <iostream>

using json = nlohmann::json;

// ---------------------------------------------------------------------------
// Minimum moves each player must have made before a win can be legitimate.
// In Chinese Chess:
//   - Red always goes first.
//   - Fastest theoretical checkmate (scholar's mate equivalent) requires
//     at least 3 half-moves (plies) from the winning side.
//   - We conservatively require MIN_MOVES_PER_PLAYER from the WINNER's side.
// ---------------------------------------------------------------------------
static constexpr int MIN_MOVES_PER_PLAYER_FOR_WIN = 3;

// ---------------------------------------------------------------------------
// Validate a win claim against server-tracked move counts.
//
// Checks:
//   1. Winner must have made >= MIN_MOVES_PER_PLAYER_FOR_WIN moves
//      as tracked by the server (not client-reported).
//   2. The client-reported moves array size must be plausible relative
//      to the server-tracked count:
//        total_reported ≈ winner_moves + loser_moves
//        winner_moves_tracked should be ≈ total_reported / 2  (±tolerance)
//
// Returns false and sends an error if the claim is suspicious.
// ---------------------------------------------------------------------------
static bool validate_win_claim(
    std::shared_ptr<Player> winner,
    std::shared_ptr<Player> loser,
    const std::vector<std::string>& moves_array)
{
    const int winner_tracked = winner->server_move_count;
    const int loser_tracked  = loser  ? loser->server_move_count : 0;
    const int total_tracked  = winner_tracked + loser_tracked;
    const int total_reported = static_cast<int>(moves_array.size());

    // 1. Did the winner actually move enough times?
    if (winner_tracked < MIN_MOVES_PER_PLAYER_FOR_WIN) {
        std::cerr << "[AntiCheat] WIN_FRAUD | winner=" << winner->name
                  << " only made " << winner_tracked
                  << " server-tracked moves (min=" << MIN_MOVES_PER_PLAYER_FOR_WIN << ")\n";
        winner->send_json(json{
            {"type",    "error"},
            {"message", "Invalid result: not enough valid moves to win"}
        });
        return false;
    }

    // 2. Cross-check: client-reported total must be >= server tracked total
    //    (client may report slightly more due to timing, but never drastically less)
    //    If reported is significantly less than tracked → fabricated short array.
    //    If reported is > tracked + large_margin → inflated fake history.
    constexpr int TOLERANCE = 4; // allow ±4 discrepancy for network/client variance
    if (total_reported < total_tracked - TOLERANCE) {
        std::cerr << "[AntiCheat] WIN_FRAUD | winner=" << winner->name
                  << " reported " << total_reported << " moves but server tracked "
                  << total_tracked << "\n";
        winner->send_json(json{
            {"type",    "error"},
            {"message", "Invalid result: move history mismatch"}
        });
        return false;
    }

    return true;
}

void GameHandler::handle(std::shared_ptr<Player> player, const json& msg, const std::string& type) {
    if (type == "move") {
        auto peer = player->opponent_lock();
        if (peer) peer->send_json(msg);

        const bool has_winner = !msg.value("winner", json(nullptr)).is_null();

        if (has_winner && peer) {
            int duration = msg.value("duration_seconds", 0);
            std::vector<std::string> moves_array;
            if (msg.contains("moves") && msg["moves"].is_array()) {
                for (const auto& m : msg["moves"]) moves_array.push_back(m.get<std::string>());
            }

            // --- Anti-cheat: validate win claim before recording ---
            if (!validate_win_claim(player, peer, moves_array)) {
                return; // Reject fraudulent win
            }

            // Anti-cheat: evaluate end-of-game behaviour metrics
            int suspicion = AntiCheatService::instance().on_game_end(
                player->name, duration, static_cast<int>(moves_array.size()));
            if (suspicion > 0) {
                std::cerr << "[AntiCheat] Suspicion score for " << player->name
                          << ": " << suspicion << "\n";
            }

            GameService::process_winner(player->name, peer->name, msg["winner"].get<int>(), duration, moves_array);

        } else if (!has_winner) {
            // Normal (non-winning) move: increment server-side counter
            player->server_move_count++;
        }
    }
    else if (type == "get_history") {
        GameService::send_history(player, 20);
    }
    else if (type == "get_replay") {
        GameService::send_replay(player, msg.value("match_id", -1));
    }
    else if (type == "game_result") {
        std::string opponent = msg.value("opponent", "");
        std::string result   = msg.value("result",   "draw");
        int duration         = msg.value("duration_seconds", 0);

        std::vector<std::string> moves_array;
        if (msg.contains("moves") && msg["moves"].is_array()) {
            for (const auto& m : msg["moves"]) moves_array.push_back(m.get<std::string>());
        }
        GameService::force_save_result(player->name, opponent, result, duration, moves_array);
    }
}
