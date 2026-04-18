#pragma once
#include <string>

namespace db {
namespace migration {

    // Lệnh: server_chinese_chess add-migration <name>
    // Quét thư mục model và sinh file .sql
    void add_migration(const std::string& name);

    // Lệnh: server_chinese_chess update-database
    // Thực thi các file .sql mới vào cơ sở dữ liệu
    void update_database();

} // namespace migration
} // namespace db
