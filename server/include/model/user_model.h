#pragma once
#include <string>
#include "db/schema_def.h"

struct users {
    db_primary_key col_type("SERIAL") int id;
    db_unique db_index col_type("VARCHAR(50) NOT NULL") std::string username;
    col_type("VARCHAR(255) NOT NULL") std::string password;
    col_type("VARCHAR(20) DEFAULT 'user'") std::string role;
    col_type("TIMESTAMP DEFAULT NULL") std::string banned_until;
    col_type("BOOLEAN DEFAULT TRUE") bool can_chat;
    col_type("BOOLEAN DEFAULT TRUE") bool can_create_room;
};
