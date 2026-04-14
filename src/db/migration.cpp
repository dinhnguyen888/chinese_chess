#include "db/migration.h"
#include "db/database.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <regex>
#include <chrono>
#include <pqxx/pqxx>
#include <vector>

namespace fs = std::filesystem;

namespace db {
namespace migration {

struct FieldDef {
    std::string name;
    std::string type;
    bool is_pk = false;
    bool is_unique = false;
    bool is_index = false;
    std::string ref_table = "";
};

struct TableDef {
    std::string name;
    std::vector<FieldDef> fields;
};

void add_migration(const std::string& name) {
    if (!fs::exists("migrations")) fs::create_directories("migrations");
    if (!fs::exists("include/model")) {
        std::cerr << "Khong tim thay thu muc include/model/\n";
        return;
    }

    auto now = std::chrono::system_clock::now();
    auto in_time_t = std::chrono::system_clock::to_time_t(now);
    char buf[100];
    std::strftime(buf, sizeof(buf), "%Y%m%d%H%M%S", std::localtime(&in_time_t));
    std::string filename = "migrations/" + std::string(buf) + "_" + name + ".sql";

    std::vector<TableDef> tables;
    std::regex struct_rx(R"reg(struct\s+(\w+))reg");
    std::regex field_word_rx(R"reg((\w+)\s*;\s*$)reg");
    std::regex pk_rx(R"reg(primary_key)reg");
    std::regex idx_rx(R"reg(index)reg");
    std::regex unique_rx(R"reg(unique)reg");
    std::regex type_rx(R"reg(type\("(.*?)"\))reg");
    std::regex o2m_rx(R"reg(one2many\("(.*?)"\))reg");

    std::vector<fs::path> model_files;
    for (const auto& entry : fs::directory_iterator("include/model")) {
        if (entry.path().extension() == ".h" && entry.path().filename() != "schema_def.h") {
            model_files.push_back(entry.path());
        }
    }
    std::sort(model_files.begin(), model_files.end());

    for (const auto& model_file : model_files) {
        const auto& model_path = model_file;
        if (model_path.extension() == ".h" && model_path.filename() != "schema_def.h") {
            std::ifstream in(model_path);
            std::string line;
            TableDef curr;
            bool in_struct = false;

            while (std::getline(in, line)) {
                // Strip comments
                auto cpos = line.find("//");
                if (cpos != std::string::npos) line = line.substr(0, cpos);

                std::smatch m;
                if (!in_struct && std::regex_search(line, m, struct_rx)) {
                    curr = TableDef();
                    curr.name = m[1].str();
                    in_struct = true;
                } else if (in_struct && line.find("}") != std::string::npos) {
                    tables.push_back(curr);
                    in_struct = false;
                } else if (in_struct && line.find(";") != std::string::npos) {
                    if (std::regex_search(line, m, field_word_rx)) {
                        FieldDef f;
                        f.name = m[1].str();
                        f.is_pk = std::regex_search(line, pk_rx);
                        f.is_unique = std::regex_search(line, unique_rx);
                        f.is_index = std::regex_search(line, idx_rx);
                        
                        std::smatch t_m;
                        if (std::regex_search(line, t_m, type_rx)) {
                            f.type = t_m[1].str();
                        } else {
                            if (line.find("int") != std::string::npos) f.type = "INTEGER";
                            else if (line.find("bool") != std::string::npos) f.type = "BOOLEAN";
                            else f.type = "VARCHAR(255)";
                        }

                        std::smatch o_m;
                        if (std::regex_search(line, o_m, o2m_rx)) {
                            f.ref_table = o_m[1].str();
                        }
                        
                        curr.fields.push_back(f);
                    }
                }
            }
        }
    }

    if (tables.empty()) {
        std::cout << "Khong co model nao de generate migration.\n";
        return;
    }

    std::ofstream out(filename);
    out << "-- MIGRATION: " << name << "\n";
    out << "-- Generated automatically from C++ models\n\n";

    for (const auto& t : tables) {
        // CREATE TABLE
        out << "CREATE TABLE IF NOT EXISTS " << t.name << " (\n";
        bool first = true;
        for (const auto& f : t.fields) {
            if (!first) out << ",\n";
            out << "    " << f.name << " " << f.type;
            if (f.is_pk) out << " PRIMARY KEY";
            if (f.is_unique) out << " UNIQUE";
            if (!f.ref_table.empty()) out << " REFERENCES " << f.ref_table;
            first = false;
        }
        out << "\n);\n\n";

        // ADD COLUMN IF NOT EXISTS cho phép ghi đè/update bảng cũ mà không drop data
        for (const auto& f : t.fields) {
            // PostgreSQL không hỗ trợ ADD PRIMARY KEY qua ADD COLUMN IF NOT EXISTS tốt (lỗi nếu đã có).
            // Do đó chỉ chạy ALTER TABLE cho các cột không phải PK.
            if (f.is_pk) continue;
            
            out << "ALTER TABLE " << t.name << " ADD COLUMN IF NOT EXISTS " << f.name << " " << f.type;
            if (!f.ref_table.empty()) out << " REFERENCES " << f.ref_table;
            out << ";\n";
            
            if (f.is_unique) {
                out << "CREATE UNIQUE INDEX IF NOT EXISTS uidx_" << t.name << "_" << f.name << " ON " << t.name << "(" << f.name << ");\n";
            }
        }

        // Tạo Index
        for (const auto& f : t.fields) {
            if (f.is_index) {
                out << "CREATE INDEX IF NOT EXISTS idx_" << t.name << "_" << f.name << " ON " << t.name << "(" << f.name << ");\n";
            }
        }
        out << "\n";
    }

    std::cout << "Tao migration thanh cong: " << filename << "\n";
}

void update_database() {
    try {
        pqxx::connection c(db::conn_str());
        pqxx::work w(c);
        
        // Khởi tạo bảng tracking migrations
        w.exec("CREATE TABLE IF NOT EXISTS schema_migrations (version VARCHAR(100) PRIMARY KEY, applied_at TIMESTAMP DEFAULT NOW());");
        w.exec("ALTER TABLE schema_migrations ALTER COLUMN version TYPE VARCHAR(100);");
        
        std::vector<std::string> executed;
        pqxx::result r_mig = w.exec("SELECT version FROM schema_migrations;");
        for (auto const& row : r_mig) {
            executed.push_back(row[0].c_str());
        }

        std::vector<std::string> files;
        if (fs::exists("migrations")) {
            for (const auto& entry : fs::directory_iterator("migrations")) {
                if (entry.path().extension() == ".sql") {
                    files.push_back(entry.path().filename().string());
                }
            }
        }
        std::sort(files.begin(), files.end());

        bool changed = false;
        for (const auto& f : files) {
            if (std::find(executed.begin(), executed.end(), f) == executed.end()) {
                std::cout << "[Migration] Dang chay: " << f << "...\n";
                std::ifstream in("migrations/" + f);
                std::stringstream buffer;
                buffer << in.rdbuf();
                
                // Tách SQL ra để sửa lỗi UNIQUE ON CONFLICT DO NOTHING vì PostgreSQL không hỗ trợ cho ALTER TABLE directly
                // Thay vì thế ta bỏ qua DO NOTHING cho constraint, nếu lỗi constraint đã tồn tại thì catch và bỏ qua
                
                try {
                    w.exec(buffer.str());
                } catch (const pqxx::sql_error& e) {
                    // Nếu lỗi do cột đã tồn tại hay constraint đã tồn tại, ta bỏ qua
                    // Tốt nhất là .sql file nên dùng các syntax an toàn nhất 
                    // Tạm thời exec nguyên cụm. Nếu lỗi => rollback và báo
                    std::cerr << "Loi khi chay migration " << f << ": " << e.what() << "\n";
                    throw;
                }
                
                w.exec_params("INSERT INTO schema_migrations (version) VALUES ($1);", f);
                changed = true;
            }
        }
        
        if (changed) {
            w.commit();
            std::cout << "Update database thanh cong!\n";
        } else {
            std::cout << "Database hien tai da la phien ban moi nhat.\n";
        }
    } catch (const std::exception& e) {
        std::cerr << "update_database loi: " << e.what() << "\n";
    }
}

} // namespace migration
} // namespace db
