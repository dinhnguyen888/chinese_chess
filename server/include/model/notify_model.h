#pragma once
#include <string>
#include "db/schema_def.h"

struct notify {
    db_primary_key col_type("SERIAL") int id;
    
};
