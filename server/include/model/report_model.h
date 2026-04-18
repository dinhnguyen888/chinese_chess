#pragma once
#include <string>
#include "db/schema_def.h"

struct reports {
    db_primary_key col_type("SERIAL") int id;
    col_type("VARCHAR(50) NOT NULL") std::string reporter;
    db_index col_type("VARCHAR(50) NOT NULL") std::string reported;
    db_one2many("match_history(id) ON DELETE SET NULL") col_type("INT") int match_id;
    col_type("TEXT NOT NULL") std::string reason;
    db_index col_type("VARCHAR(20) DEFAULT 'pending'") std::string status;
    db_index col_type("TIMESTAMP DEFAULT NOW()") std::string created_at;
};
