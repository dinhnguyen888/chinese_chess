#include "service/game_service.h"
#include "db/match_db.h"
#include "type/player.h"

using json = nlohmann::json;

void GameService::process_winner(const std::string& my_name, const std::string& opp_name,
                                 int winner_side, int duration_seconds, 
                                 const std::vector<std::string>& moves_array) {
    if (my_name.empty() || opp_name.empty()) return;

    std::string my_result, opp_result;
    if (winner_side == 1) {
        my_result = "win"; opp_result = "lose";
    } else if (winner_side == -1) {
        my_result = "lose"; opp_result = "win";
    } else {
        my_result = opp_result = "draw";
    }

    db::match::save_match(my_name, opp_name, my_result, duration_seconds, moves_array);
    db::match::save_match(opp_name, my_name, opp_result, duration_seconds, moves_array);
}

void GameService::force_save_result(const std::string& my_name, const std::string& opp_name, 
                                    const std::string& result, int duration_seconds, 
                                    const std::vector<std::string>& moves_array) {
    if (my_name.empty() || opp_name.empty()) return;

    db::match::save_match(my_name, opp_name, result, duration_seconds, moves_array);
    std::string opp_result = (result == "win") ? "lose" : (result == "lose" ? "win" : "draw");
    db::match::save_match(opp_name, my_name, opp_result, duration_seconds, moves_array);
}

void GameService::send_history(std::shared_ptr<Player> player, int limit) {
    if (player->name.empty()) return;
    auto records = db::match::get_history(player->name, limit);
    json arr = json::array();
    for (const auto& r : records) {
        arr.push_back({
            {"id", r.id}, {"opponent", r.opponent}, {"result", r.result},
            {"played_at", r.played_at}, {"duration_seconds", r.duration_seconds}
        });
    }
    player->send_json(json{{"type", "history_list"}, {"records", arr}});
}

void GameService::send_replay(std::shared_ptr<Player> player, int match_id) {
    if (match_id != -1) {
        auto moves = db::match::get_match_moves(match_id);
        json moves_arr = json::array();
        for (const auto& m : moves) moves_arr.push_back(m);
        player->send_json(json{{"type", "replay_data"}, {"match_id", match_id}, {"moves", moves_arr}});
    }
}
