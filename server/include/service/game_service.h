#pragma once
#include <memory>
#include <string>
#include <vector>
#include "type/player.h"
#include "model/match_model.h"

class GameService {
public:
    struct MatchRecord : public match_history {
        bool has_report;
    };

    static void process_winner(const std::string& my_name, const std::string& opp_name,
                               int winner_side, int duration_seconds,
                               const std::vector<std::string>& moves_array);

    static void force_save_result(const std::string& my_name, const std::string& opp_name,
                                  const std::string& result, int duration_seconds,
                                  const std::vector<std::string>& moves_array);

    static std::vector<MatchRecord> get_history(const std::string& username, int limit = 20);

    static void send_history(std::shared_ptr<Player> player, int limit = 20);
    static void send_replay(std::shared_ptr<Player> player, int match_id);
};
