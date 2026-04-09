#pragma once
#include <memory>
#include <string>
#include <nlohmann/json.hpp>

class Player;

class GameHandler {
public:
    static void handle(std::shared_ptr<Player> player, const nlohmann::json& msg, const std::string& type);
};
