#pragma once
#include <memory>
#include <string>

class Player;

class AuthService {
public:
    static void verify_jwt(std::shared_ptr<Player> player, const std::string& token);
    static void register_user(std::shared_ptr<Player> player, const std::string& username, const std::string& password);
    static void login_user(std::shared_ptr<Player> player, const std::string& username, const std::string& password);
};
