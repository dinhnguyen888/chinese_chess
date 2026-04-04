#pragma once
#include <string>
#include <vector>

struct MatchRecord {
    int id;
    std::string opponent;
    std::string result;   // "win" | "lose" | "draw"
    std::string played_at;
    int duration_seconds;
};

class Database {
public:
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    static Database& get_instance() {
        static Database instance;
        return instance;
    }
    
    void auto_create_db(const std::string& host, const std::string& port, const std::string& user, const std::string& password, const std::string& dbname);

    // Tạo bảng nếu chưa có
    void init();
    
    // Kết nối với CSDL Postgres
    bool init_connection(const std::string& conn_str);

    // Trả về true nếu thành công, false nếu username đã tồn tại hoặc lỗi
    bool register_user(const std::string& username, const std::string& password);

    // Trả về true nếu login đúng
    bool login_user(const std::string& username, const std::string& password);

    // Lưu kết quả một trận đấu kèm danh sách các nước đi
    bool save_match(const std::string& username, const std::string& opponent,
                    const std::string& result, int duration_seconds, const std::vector<std::string>& moves);

    // Lấy danh sách lịch sử đấu của người chơi (mới nhất trước)
    std::vector<MatchRecord> get_history(const std::string& username, int limit = 20);

    // Lấy danh sách các nước đi của một trận đấu
    std::vector<std::string> get_match_moves(int match_id);

private:
    Database() = default;
    std::string conn_str_;
};
