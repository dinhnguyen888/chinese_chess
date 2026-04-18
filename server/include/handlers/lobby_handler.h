#pragma once
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

class Player;
class MatchLobbyService;

class LobbyHandler {
public:
    static void handle(std::shared_ptr<Player> player, MatchLobbyService& lobby, const nlohmann::json& msg, const std::string& type);
};
