#include "handlers/game_handler.h"
#include "type/player.h"
#include "service/game_service.h"

using json = nlohmann::json;

void GameHandler::handle(std::shared_ptr<Player> player, const json& msg, const std::string& type) {
    if (type == "move") {
        auto peer = player->opponent_lock();
        if (peer) peer->send_json(msg);

        if (!msg.value("winner", json(nullptr)).is_null() && peer) {
            int duration = msg.value("duration_seconds", 0);
            std::vector<std::string> moves_array;
            if (msg.contains("moves") && msg["moves"].is_array()) {
                for (const auto& m : msg["moves"]) moves_array.push_back(m.get<std::string>());
            }
            GameService::process_winner(player->name, peer->name, msg["winner"].get<int>(), duration, moves_array);
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
        std::string result = msg.value("result", "draw");
        int duration = msg.value("duration_seconds", 0);
        
        std::vector<std::string> moves_array;
        if (msg.contains("moves") && msg["moves"].is_array()) {
            for (const auto& m : msg["moves"]) moves_array.push_back(m.get<std::string>());
        }
        GameService::force_save_result(player->name, opponent, result, duration, moves_array);
    }
}
