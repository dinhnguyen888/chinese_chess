#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>
#include "type/match_lobby_types.h"

class Player;

class MatchLobbyService {
public:
    void try_pair(std::shared_ptr<Player> player);
    void cancel_waiting(const std::shared_ptr<Player>& player);

    void create_room(std::shared_ptr<Player> player, const std::string& room_name, bool auto_start);
    void join_room(std::shared_ptr<Player> player, const std::string& room_id);
    void start_game(std::shared_ptr<Player> player, const std::string& room_id);
    void get_room_list(std::shared_ptr<Player> player);
    void search_room(std::shared_ptr<Player> player, const std::string& query);
    void chat_in_room(std::shared_ptr<Player> sender, const std::string& message);

private:
    void leave_room_internal(const std::shared_ptr<Player>& player);

    std::mutex mutex_;
    MatchLobbyState state_;
};
