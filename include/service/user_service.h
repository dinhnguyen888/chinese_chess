#pragma once
#include <string>
#include <optional>
#include <vector>
#include "model/user_model.h"

class UserService {
public:
    static bool register_user(const std::string& username, const std::string& password);
    static std::optional<users> login_user(const std::string& username, const std::string& password);
    static std::optional<users> get_user_by_username(const std::string& username);
    
    static std::pair<std::vector<users>, int> get_all_users(int page = 1, int page_size = 20);
    static bool create_user(const std::string& username, const std::string& password, const std::string& role);
    static bool update_user(int id, const std::string& username, const std::string& password, const std::string& role);
    static bool delete_user(int id);

    static bool apply_punishment(const std::string& username, int ban_days, bool can_chat, bool can_create_room);
};
