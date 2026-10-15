#pragma once
#include "db/Database.h"

namespace rtrs {

// Creates all tables if they do not exist. Safe to call on every start-up.
void initSchema(Database& db);

}  // namespace rtrs
