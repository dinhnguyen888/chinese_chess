#pragma once
#include <memory>
#include <mutex>
#include <string>
#include <nlohmann/json.hpp>
#include "type/match_lobby_types.h"

class Player;

class MatchLobbyService : public std::enable_shared_from_this<MatchLobbyService> {
public:
    void register_player(const std::string& name, std::shared_ptr<Player> player);
    void unregister_player(const std::string& name);
    
    // Bắn thông báo phạt về cho người chơi online
    void notify_punishment(const std::string& target, const std::string& reason, const std::string& reporter, int ban_days, bool can_chat, bool can_create_room);

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
    std::unordered_map<std::string, std::shared_ptr<Player>> online_players_;
};

extern std::shared_ptr<MatchLobbyService> g_lobby;
