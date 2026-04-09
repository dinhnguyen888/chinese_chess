#include "handlers/auth_handler.h"
#include "service/auth_service.h"

using json = nlohmann::json;

void AuthHandler::handle(std::shared_ptr<Player> player, const json& msg, const std::string& type) {
    if (type == "verify_jwt") {
        AuthService::verify_jwt(player, msg.value("token", ""));
    } else if (type == "register") {
        AuthService::register_user(player, msg.value("username", ""), msg.value("password", ""));
    } else if (type == "login") {
        AuthService::login_user(player, msg.value("username", ""), msg.value("password", ""));
    }
}
