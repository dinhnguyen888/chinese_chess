#include "db/database.h"
#include "db/migration.h"
#include <pqxx/pqxx>
#include <iostream>
#include <vector>
#include <functional>

static std::string s_conn_str;

namespace db {
const std::string& conn_str() {
    return s_conn_str;
}

bool auto_create(const std::string& host, const std::string& port,
                 const std::string& user, const std::string& password,
                 const std::string& dbname) {
    try {
        std::string admin_conn = "host=" + host + " port=" + port
                               + " dbname=postgres user=" + user
                               + " password=" + password;
        pqxx::connection c(admin_conn);
        pqxx::nontransaction tx(c);

        pqxx::result r = tx.exec_params("SELECT 1 FROM pg_database WHERE datname=$1", dbname);
        if (r.empty()) {
            std::cout << "Database '" << dbname << "' chua ton tai, dang tao moi...\n";
            tx.exec("CREATE DATABASE " + dbname);
            std::cout << "Tao Database '" << dbname << "' thanh cong!\n";
        }
        return true;
    } catch (const std::exception& e) {
        std::cerr << "auto_create error: " << e.what() << "\n";
        return false;
    }
}

bool connect(const std::string& conn_str) {
    try {
        pqxx::connection c(conn_str);
        if (c.is_open()) {
            s_conn_str = conn_str;
            return true;
        }
        return false;
    } catch (const std::exception& e) {
        std::cerr << "DB connect error: " << e.what() << "\n";
        return false;
    }
}

void init_schema() {
    try {
        // Run completely dynamic migrations (ORMs style)
        db::migration::update_database();

        pqxx::connection c(s_conn_str);
        // Post-migration: Create default admin (since default rows shouldn't necessarily be in schema)
        {
            pqxx::work w2(c);
            pqxx::result r = w2.exec_params("SELECT 1 FROM users WHERE username=$1", "admin");
            if (r.empty()) {
                w2.exec_params("INSERT INTO users (username, password, role) VALUES ($1, $2, $3)",
                             "admin", "admin123", "admin");
                w2.commit();
                std::cout << "[Init] Created default admin account (admin/admin123)\n";
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "init_schema error: " << e.what() << "\n";
    }
}

} 
