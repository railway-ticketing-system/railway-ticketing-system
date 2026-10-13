#pragma once
#include <httplib.h>

#include "auth/PasswordHasher.h"
#include "db/Database.h"

namespace rtrs {

// Registers every REST endpoint on the server. Kept separate from main() so
// tests can build the same app against an in-memory database.
void registerRoutes(httplib::Server& server, Database& db,
                    int pbkdf2Iterations = PasswordHasher::kDefaultIterations);

}  // namespace rtrs
