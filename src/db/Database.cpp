#include "db/Database.h"
#include <pqxx/pqxx>
#include <iostream>

void Database::auto_create_db(const std::string& host, const std::string& port, const std::string& user, const std::string& password, const std::string& dbname) {
    try {
        std::string default_conn = "host=" + host + " port=" + port + " dbname=postgres user=" + user + " password=" + password;
        pqxx::connection default_c(default_conn);
        pqxx::nontransaction ntx(default_c);
        
        pqxx::result res = ntx.exec_params("SELECT 1 FROM pg_database WHERE datname=$1", dbname);
        if (res.empty()) {
            std::cout << "Database '" << dbname << "' chua ton tai. Dang tu dong tao moi...\n";
            ntx.exec("CREATE DATABASE " + dbname);
            std::cout << "Tao Database '" << dbname << "' thanh cong!\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "Loi khi auto-create DB: " << e.what() << "\n(Neu Database da ton tai thi ban co the bo qua loi nay)\n";
    }
}

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
        w.exec(
            "CREATE TABLE IF NOT EXISTS match_history ("
            "id SERIAL PRIMARY KEY,"
            "username VARCHAR(50) NOT NULL,"
            "opponent VARCHAR(50) NOT NULL,"
            "result VARCHAR(10) NOT NULL,"
            "duration_seconds INT DEFAULT 0,"
            "played_at TIMESTAMP DEFAULT NOW()"
            ");"
        );
        w.exec(
            "CREATE TABLE IF NOT EXISTS match_moves ("
            "id SERIAL PRIMARY KEY,"
            "match_id INT NOT NULL REFERENCES match_history(id) ON DELETE CASCADE,"
            "move_index INT NOT NULL,"
            "move_data VARCHAR(150) NOT NULL"
            ");"
        );
        w.commit();
        std::cout << "Database initialized (users + match_history + match_moves tables checked).\n";
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
    if (username.length() > 10 || username.empty()) return false;
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
    if (username.length() > 10 || username.empty()) return false;
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

bool Database::save_match(const std::string& username, const std::string& opponent,
                          const std::string& result, int duration_seconds, const std::vector<std::string>& moves) {
    try {
        if (conn_str_.empty()) return false;
        pqxx::connection c(conn_str_);
        pqxx::work w(c);
        
        // Insert vào match_history và lấy id trả về (sử dụng RETURNING id)
        pqxx::result res = w.exec_params(
            "INSERT INTO match_history (username, opponent, result, duration_seconds) "
            "VALUES ($1, $2, $3, $4) RETURNING id;",
            username, opponent, result, duration_seconds
        );
        
        if (res.empty()) return false;
        int match_id = res[0][0].as<int>();

        // Lặp qua mảng moves và insert vào match_moves
        for (size_t i = 0; i < moves.size(); ++i) {
            w.exec_params(
                "INSERT INTO match_moves (match_id, move_index, move_data) VALUES ($1, $2, $3);",
                match_id, (int)i, moves[i]
            );
        }

        w.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "DB SaveMatch Error: " << e.what() << "\n";
        return false;
    }
}

std::vector<MatchRecord> Database::get_history(const std::string& username, int limit) {
    std::vector<MatchRecord> records;
    try {
        if (conn_str_.empty()) return records;
        pqxx::connection c(conn_str_);
        pqxx::nontransaction w(c);
        pqxx::result r = w.exec_params(
            "SELECT id, opponent, result, TO_CHAR(played_at, 'DD/MM/YYYY HH24:MI') as played_at, duration_seconds "
            "FROM match_history WHERE username=$1 ORDER BY played_at DESC LIMIT $2;",
            username, limit
        );
        for (const auto& row : r) {
            MatchRecord rec;
            rec.id = row["id"].as<int>();
            rec.opponent = row["opponent"].c_str();
            rec.result = row["result"].c_str();
            rec.played_at = row["played_at"].c_str();
            rec.duration_seconds = row["duration_seconds"].as<int>();
            records.push_back(rec);
        }
    } catch (const std::exception& e) {
        std::cerr << "DB GetHistory Error: " << e.what() << "\n";
    }
    return records;
}

std::vector<std::string> Database::get_match_moves(int match_id) {
    std::vector<std::string> moves;
    try {
        if (conn_str_.empty()) return moves;
        pqxx::connection c(conn_str_);
        pqxx::nontransaction w(c);
        pqxx::result r = w.exec_params(
            "SELECT move_data FROM match_moves WHERE match_id=$1 ORDER BY move_index ASC;",
            match_id
        );
        for (const auto& row : r) {
            moves.push_back(row["move_data"].c_str());
        }
    } catch (const std::exception& e) {
        std::cerr << "DB GetMatchMoves Error: " << e.what() << "\n";
    }
    return moves;
}
