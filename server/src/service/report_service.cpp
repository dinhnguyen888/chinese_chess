#include "service/report_service.h"
#include "db/database.h"
#include <pqxx/pqxx>
#include <iostream>

bool ReportService::create_report(const std::string& reporter, const std::string& reported, int match_id, const std::string& reason) {
    try {
        pqxx::connection c(db::conn_str());
        pqxx::work w(c);
        if (match_id > 0) {
            w.exec_params("INSERT INTO reports (reporter, reported, match_id, reason) VALUES ($1, $2, $3, $4);",
                          reporter, reported, match_id, reason);
        } else {
            w.exec_params("INSERT INTO reports (reporter, reported, reason) VALUES ($1, $2, $3);",
                          reporter, reported, reason);
        }
        w.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "report::create error: " << e.what() << "\n";
        return false;
    }
}

std::pair<std::vector<reports>, int> ReportService::get_all_reports(int page, int page_size) {
    std::vector<reports> report_list;
    int total_count = 0;
    try {
        pqxx::connection c(db::conn_str());
        pqxx::nontransaction w(c);
        
        // Get total count
        pqxx::result r_count = w.exec("SELECT COUNT(*) FROM reports;");
        total_count = r_count[0][0].as<int>();

        // Get paged data
        int offset = (page - 1) * page_size;
        pqxx::result r = w.exec_params(
            "SELECT id, reporter, reported, COALESCE(match_id, 0) as match_id, reason, status, "
            "TO_CHAR(created_at, 'DD/MM/YYYY HH24:MI') as created_at "
            "FROM reports ORDER BY id DESC LIMIT $1 OFFSET $2;",
            page_size, offset
        );

        for (const auto& row : r) {
            report_list.push_back({
                row["id"].as<int>(),
                row["reporter"].c_str(),
                row["reported"].c_str(),
                row["match_id"].as<int>(),
                row["reason"].c_str(),
                row["status"].c_str(),
                row["created_at"].c_str()
            });
        }
    } catch (const std::exception& e) {
        std::cerr << "report::get_all error: " << e.what() << "\n";
    }
    return {report_list, total_count};
}

bool ReportService::update_report_status(int id, const std::string& status) {
    try {
        pqxx::connection c(db::conn_str());
        pqxx::work w(c);
        w.exec_params("UPDATE reports SET status=$1 WHERE id=$2;", status, id);
        w.commit();
        return true;
    } catch (...) { return false; }
}
