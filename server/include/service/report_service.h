#pragma once
#include <string>
#include <vector>

#include "model/report_model.h"

class ReportService {
public:
    static bool create_report(const std::string& reporter, const std::string& reported, int match_id, const std::string& reason);
    static std::pair<std::vector<reports>, int> get_all_reports(int page = 1, int page_size = 20);
    static bool update_report_status(int id, const std::string& status);
};
