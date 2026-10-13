#include "db/Schema.h"

namespace rtrs {

void initSchema(Database& db) {
    db.execute(
        "CREATE TABLE IF NOT EXISTS users ("
        " id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        " name          TEXT NOT NULL,"
        " email         TEXT NOT NULL UNIQUE,"
        " phone         TEXT NOT NULL,"
        " password_hash TEXT NOT NULL,"
        " role          TEXT NOT NULL DEFAULT 'PASSENGER' CHECK (role IN ('PASSENGER','ADMIN')),"
        " created_at    TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))"
        ")");
    // Only a SHA-256 of the bearer token is stored, so a leaked database cannot be replayed.
    db.execute(
        "CREATE TABLE IF NOT EXISTS sessions ("
        " token_hash TEXT PRIMARY KEY,"
        " user_id    INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
        " expires_at INTEGER NOT NULL"
        ")");
}

}  // namespace rtrs
