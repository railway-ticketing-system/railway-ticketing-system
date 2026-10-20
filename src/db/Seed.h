#pragma once
#include "db/Database.h"

namespace rtrs {

// Inserts demo trains, stops and fares when the trains table is empty.
void seedDemoData(Database& db);

}  // namespace rtrs
