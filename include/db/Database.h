#pragma once
#include <string>

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

private:
    Database() = default;
    std::string conn_str_;
};
