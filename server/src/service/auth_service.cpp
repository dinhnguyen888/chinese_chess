#include "service/auth_service.h"
#include "type/player.h"
#include "service/user_service.h"
#include "utils/jwt_utils.h"
#include <nlohmann/json.hpp>

using json = nlohmann::json;

void AuthService::verify_jwt(std::shared_ptr<Player> player, const std::string& token) {
    std::string user = utils::jwt::decode_and_verify(token);
    if (!user.empty()) {
        player->name = user;
        player->send_json(json{{"type", "auth_success"}, {"username", user}, {"action", "verify"}});
    } else {
        player->send_json(json{{"type", "auth_fail"}, {"message", "Phiên đăng nhập hết hạn hoặc không hợp lệ"}});
    }
}

void AuthService::register_user(std::shared_ptr<Player> player, const std::string& username, const std::string& password) {
    if (UserService::register_user(username, password)) {
        player->name = username;
        std::string token = utils::jwt::generate_token(username);
        player->send_json(json{{"type", "auth_success"}, {"username", username}, {"action", "register"}, {"token", token}});
    } else {
        player->send_json(json{{"type", "error"}, {"message", "Tên đăng nhập đã tồn tại hoặc có lỗi xảy ra"}});
    }
}

void AuthService::login_user(std::shared_ptr<Player> player, const std::string& username, const std::string& password) {
    auto user_opt = UserService::login_user(username, password);
    if (user_opt) {
        player->name = user_opt->username;
        std::string token = utils::jwt::generate_token(player->name);
        player->send_json(json{{"type", "auth_success"}, {"username", player->name}, {"action", "login"}, {"token", token}});
    } else {
        player->send_json(json{{"type", "error"}, {"message", "Sai tên đăng nhập hoặc mật khẩu"}});
    }
}
