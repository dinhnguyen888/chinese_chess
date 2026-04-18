#include "service/user_service.h"
#include "db/database.h"
#include <pqxx/pqxx>
#include <iostream>

bool UserService::register_user(const std::string& username, const std::string& password) {
    if (username.empty() || username.length() > 20) return false;
    try {
        pqxx::connection c(db::conn_str());
        pqxx::work w(c);
        w.exec_params("INSERT INTO users (username, password, role) VALUES ($1, $2, 'user');",
                      username, password);
        w.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "user::register error: " << e.what() << "\n";
        return false;
    }
}

std::optional<users> UserService::login_user(const std::string& username, const std::string& password) {
    if (username.empty() || username.length() > 20) return std::nullopt;
    try {
        pqxx::connection c(db::conn_str());
        pqxx::nontransaction w(c);
        pqxx::result r = w.exec_params(
            "SELECT id, username, role, TO_CHAR(banned_until, 'YYYY-MM-DD HH24:MI:SS') as banned_until, can_chat, can_create_room"
            " FROM users WHERE username=$1 AND password=$2;",
            username, password
        );
        if (r.empty()) return std::nullopt;
        
        users user;
        user.id = r[0]["id"].as<int>();
        user.username = r[0]["username"].c_str();
        user.role = r[0]["role"].c_str();
        user.banned_until = r[0]["banned_until"].is_null() ? "" : r[0]["banned_until"].c_str();
        user.can_chat = r[0]["can_chat"].as<bool>();
        user.can_create_room = r[0]["can_create_room"].as<bool>();
        return user;
    } catch (const std::exception& e) {
        std::cerr << "user::login error: " << e.what() << "\n";
        return std::nullopt;
    }
}

std::optional<users> UserService::get_user_by_username(const std::string& username) {
    if (username.empty()) return std::nullopt;
    try {
        pqxx::connection c(db::conn_str());
        pqxx::nontransaction w(c);
        pqxx::result r = w.exec_params(
            "SELECT id, username, role, TO_CHAR(banned_until, 'YYYY-MM-DD HH24:MI:SS') as banned_until, can_chat, can_create_room"
            " FROM users WHERE username=$1;",
            username
        );
        if (r.empty()) return std::nullopt;
        
        users user;
        user.id = r[0]["id"].as<int>();
        user.username = r[0]["username"].c_str();
        user.role = r[0]["role"].c_str();
        user.banned_until = r[0]["banned_until"].is_null() ? "" : r[0]["banned_until"].c_str();
        user.can_chat = r[0]["can_chat"].as<bool>();
        user.can_create_room = r[0]["can_create_room"].as<bool>();
        return user;
    } catch (const std::exception& e) {
        std::cerr << "user::get_user_by_username error: " << e.what() << "\n";
        return std::nullopt;
    }
}

std::pair<std::vector<users>, int> UserService::get_all_users(int page, int page_size) {
    std::vector<users> users_list;
    int total_count = 0;
    try {
        pqxx::connection c(db::conn_str());
        pqxx::nontransaction w(c);
        
        // Get total count
        pqxx::result r_count = w.exec("SELECT COUNT(*) FROM users;");
        total_count = r_count[0][0].as<int>();

        // Get paged data
        int offset = (page - 1) * page_size;
        pqxx::result r = w.exec_params(
            "SELECT id, username, role, TO_CHAR(banned_until, 'YYYY-MM-DD HH24:MI:SS') as banned_until, can_chat, can_create_room "
            "FROM users ORDER BY id ASC LIMIT $1 OFFSET $2;",
            page_size, offset
        );

        for (auto const& row : r) {
            users u;
            u.id = row["id"].as<int>();
            u.username = row["username"].c_str();
            u.role = row["role"].c_str();
            u.banned_until = row["banned_until"].is_null() ? "" : row["banned_until"].c_str();
            u.can_chat = row["can_chat"].as<bool>();
            u.can_create_room = row["can_create_room"].as<bool>();
            users_list.push_back(u);
        }
    } catch (const std::exception& e) {
        std::cerr << "user::get_all_users error: " << e.what() << "\n";
    }
    return {users_list, total_count};
}

bool UserService::create_user(const std::string& username, const std::string& password, const std::string& role) {
    try {
        pqxx::connection c(db::conn_str());
        pqxx::work w(c);
        w.exec_params("INSERT INTO users (username, password, role) VALUES ($1, $2, $3);",
                      username, password, role);
        w.commit();
        return true;
    } catch (...) { return false; }
}

bool UserService::update_user(int id, const std::string& username, const std::string& password, const std::string& role) {
    try {
        pqxx::connection c(db::conn_str());
        pqxx::work w(c);
        if (password.empty()) {
            w.exec_params("UPDATE users SET username=$1, role=$2 WHERE id=$3;", username, role, id);
        } else {
            w.exec_params("UPDATE users SET username=$1, password=$2, role=$3 WHERE id=$4;", username, password, role, id);
        }
        w.commit();
        return true;
    } catch (...) { return false; }
}

bool UserService::delete_user(int id) {
    try {
        pqxx::connection c(db::conn_str());
        pqxx::work w(c);
        w.exec_params("DELETE FROM users WHERE id=$1;", id);
        w.commit();
        return true;
    } catch (...) { return false; }
}

bool UserService::apply_punishment(const std::string& username, int ban_days, bool can_chat, bool can_create_room) {
    try {
        pqxx::connection c(db::conn_str());
        pqxx::work w(c);
        
        std::string banned_until_sql = "NULL";
        if (ban_days == -1) { // Permanent
            banned_until_sql = "NOW() + interval '99 years'";
        } else if (ban_days > 0) {
            banned_until_sql = "NOW() + interval '" + std::to_string(ban_days) + " days'";
        }

        std::string query = "UPDATE users SET banned_until=" + (ban_days == 0 ? "NULL" : banned_until_sql) + 
                          ", can_chat=$1, can_create_room=$2 WHERE username=$3;";
        w.exec_params(query, can_chat, can_create_room, username);
        w.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "apply_punishment error: " << e.what() << "\n";
        return false;
    }
}
