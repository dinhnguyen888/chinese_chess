#include "db/Database.h"
#include <pqxx/pqxx>
#include <iostream>

void Database::init() {
    try {
        if (conn_str_.empty()) {
            std::cerr << "Warning: No database connection string provided.\n";
            return;
        }
        pqxx::connection c(conn_str_);
        pqxx::work w(c);
        w.exec(
            "CREATE TABLE IF NOT EXISTS users ("
            "id SERIAL PRIMARY KEY,"
            "username VARCHAR(50) UNIQUE NOT NULL,"
            "password VARCHAR(255) NOT NULL"
            ");"
        );
        w.commit();
        std::cout << "Database initialized (users table checked).\n";
    } catch (const std::exception& e) {
        std::cerr << "DB Init Error: " << e.what() << "\n";
    }
}

bool Database::init_connection(const std::string& conn_str) {
    conn_str_ = conn_str;
    try {
        pqxx::connection c(conn_str_);
        return c.is_open();
    } catch (const std::exception& e) {
        std::cerr << "DB Conn Error: " << e.what() << "\n";
        return false;
    }
}

bool Database::register_user(const std::string& username, const std::string& password) {
    try {
        if (conn_str_.empty()) return false;
        pqxx::connection c(conn_str_);
        pqxx::work w(c);
        
        // Simple plain text password for demo, production -> hashing required
        w.exec_params("INSERT INTO users (username, password) VALUES ($1, $2);", username, password);
        w.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "DB Register Error: " << e.what() << "\n";
        return false;
    }
}

bool Database::login_user(const std::string& username, const std::string& password) {
    try {
        if (conn_str_.empty()) return false;
        pqxx::connection c(conn_str_);
        pqxx::nontransaction w(c);
        
        pqxx::result r = w.exec_params("SELECT id FROM users WHERE username = $1 AND password = $2;", username, password);
        return !r.empty();
    } catch (const std::exception& e) {
        std::cerr << "DB Login Error: " << e.what() << "\n";
        return false;
    }
}
