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
    db.execute(
        "CREATE TABLE IF NOT EXISTS trains ("
        " id     INTEGER PRIMARY KEY AUTOINCREMENT,"
        " number TEXT NOT NULL UNIQUE,"
        " name   TEXT NOT NULL"
        ")");
    db.execute(
        "CREATE TABLE IF NOT EXISTS train_stops ("
        " id             INTEGER PRIMARY KEY AUTOINCREMENT,"
        " train_id       INTEGER NOT NULL REFERENCES trains(id) ON DELETE CASCADE,"
        " station        TEXT NOT NULL,"
        " arrival_time   TEXT,"
        " departure_time TEXT,"
        " day_offset     INTEGER NOT NULL DEFAULT 0,"
        " seq            INTEGER NOT NULL,"
        " UNIQUE (train_id, seq)"
        ")");
    db.execute("CREATE INDEX IF NOT EXISTS idx_train_stops_station ON train_stops (station)");
    db.execute(
        "CREATE TABLE IF NOT EXISTS train_classes ("
        " train_id    INTEGER NOT NULL REFERENCES trains(id) ON DELETE CASCADE,"
        " class_code  TEXT NOT NULL,"
        " class_name  TEXT NOT NULL,"
        " fare        INTEGER NOT NULL CHECK (fare > 0),"
        " total_seats INTEGER NOT NULL CHECK (total_seats > 0),"
        " PRIMARY KEY (train_id, class_code)"
        ")");
    db.execute(
        "CREATE TABLE IF NOT EXISTS seat_reservations ("
        " id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        " train_id    INTEGER NOT NULL,"
        " travel_date TEXT NOT NULL,"
        " class_code  TEXT NOT NULL,"
        " seats       INTEGER NOT NULL CHECK (seats > 0),"
        " FOREIGN KEY (train_id, class_code)"
        "   REFERENCES train_classes(train_id, class_code) ON DELETE CASCADE"
        ")");
}

}  // namespace rtrs
