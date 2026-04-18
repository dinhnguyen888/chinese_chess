#pragma once
#include <string>
#include "db/schema_def.h"

struct match_history {
    db_primary_key col_type("SERIAL") int id;
    db_index col_type("VARCHAR(50) NOT NULL") std::string username;
    col_type("VARCHAR(50) NOT NULL") std::string opponent;
    col_type("VARCHAR(10) NOT NULL") std::string result;
    col_type("INT DEFAULT 0") int duration_seconds;
    db_index col_type("TIMESTAMP DEFAULT NOW()") std::string played_at;
};

struct match_moves {
    db_primary_key col_type("SERIAL") int id;
    db_one2many("match_history(id) ON DELETE CASCADE") col_type("INT NOT NULL") int match_id;
    col_type("INT NOT NULL") int move_index;
    col_type("VARCHAR(150) NOT NULL") std::string move_data;
};
