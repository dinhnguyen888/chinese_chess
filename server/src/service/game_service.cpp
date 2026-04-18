#include "service/game_service.h"
#include "type/player.h"
#include "db/database.h"
#include <pqxx/pqxx>
#include <iostream>

using json = nlohmann::json;

static bool save_match_db(const std::string& username, const std::string& opponent,
                          const std::string& result, int duration_seconds,
                          const std::vector<std::string>& moves) {
    try {
        pqxx::connection c(db::conn_str());
        pqxx::work w(c);

        pqxx::result res = w.exec_params(
            "INSERT INTO match_history (username, opponent, result, duration_seconds)"
            " VALUES ($1, $2, $3, $4) RETURNING id;",
            username, opponent, result, duration_seconds
        );
        if (res.empty()) return false;
        int match_id = res[0][0].as<int>();

        for (size_t i = 0; i < moves.size(); ++i) {
            w.exec_params(
                "INSERT INTO match_moves (match_id, move_index, move_data) VALUES ($1, $2, $3);",
                match_id, (int)i, moves[i]
            );
        }

        w.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "match::save error: " << e.what() << "\n";
        return false;
    }
}

std::vector<GameService::MatchRecord> GameService::get_history(const std::string& username, int limit) {
    std::vector<GameService::MatchRecord> records;
    try {
        pqxx::connection c(db::conn_str());
        pqxx::nontransaction w(c);
        pqxx::result r = w.exec_params(
            "SELECT id, opponent, result,"
            "       TO_CHAR(played_at, 'DD/MM/YYYY HH24:MI') AS played_at,"
            "       duration_seconds,"
            "       EXISTS(SELECT 1 FROM reports WHERE match_id = match_history.id) as has_report"
            " FROM match_history"
            " WHERE username=$1"
            " ORDER BY played_at DESC"
            " LIMIT $2;",
            username, limit
        );
        for (const auto& row : r) {
            GameService::MatchRecord rec;
            rec.id               = row["id"].as<int>();
            rec.opponent         = row["opponent"].c_str();
            rec.result           = row["result"].c_str();
            rec.played_at        = row["played_at"].c_str();
            rec.duration_seconds = row["duration_seconds"].as<int>();
            rec.has_report       = row["has_report"].as<bool>();
            records.push_back(rec);
        }
    } catch (const std::exception& e) {
        std::cerr << "match::get_history error: " << e.what() << "\n";
    }
    return records;
}

static std::vector<std::string> get_match_moves_db(int match_id) {
    std::vector<std::string> moves;
    try {
        pqxx::connection c(db::conn_str());
        pqxx::nontransaction w(c);
        pqxx::result r = w.exec_params(
            "SELECT move_data"
            " FROM match_moves"
            " WHERE match_id=$1"
            " ORDER BY move_index ASC;",
            match_id
        );
        for (const auto& row : r) {
            moves.push_back(row["move_data"].c_str());
        }
    } catch (const std::exception& e) {
        std::cerr << "match::get_match_moves error: " << e.what() << "\n";
    }
    return moves;
}

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

    save_match_db(my_name, opp_name, my_result, duration_seconds, moves_array);
    save_match_db(opp_name, my_name, opp_result, duration_seconds, moves_array);
}

void GameService::force_save_result(const std::string& my_name, const std::string& opp_name, 
                                    const std::string& result, int duration_seconds, 
                                    const std::vector<std::string>& moves_array) {
    if (my_name.empty() || opp_name.empty()) return;

    save_match_db(my_name, opp_name, result, duration_seconds, moves_array);
    std::string opp_result = (result == "win") ? "lose" : (result == "lose" ? "win" : "draw");
    save_match_db(opp_name, my_name, opp_result, duration_seconds, moves_array);
}

void GameService::send_history(std::shared_ptr<Player> player, int limit) {
    if (player->name.empty()) return;
    auto records = get_history(player->name, limit);
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
        auto moves = get_match_moves_db(match_id);
        json moves_arr = json::array();
        for (const auto& m : moves) moves_arr.push_back(m);
        player->send_json(json{{"type", "replay_data"}, {"match_id", match_id}, {"moves", moves_arr}});
    }
}
