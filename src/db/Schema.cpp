#include "db/Schema.h"

namespace rtrs {

namespace {

bool hasColumn(Database& db, const std::string& table, const std::string& column) {
    for (const auto& row : db.query("PRAGMA table_info(" + table + ")"))
        if (row.size() > 1 && row[1] == column) return true;
    return false;
}

}  // namespace

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
        "CREATE TABLE IF NOT EXISTS bookings ("
        " id              INTEGER PRIMARY KEY AUTOINCREMENT,"
        " pnr             TEXT NOT NULL UNIQUE,"
        " user_id         INTEGER NOT NULL REFERENCES users(id) ON DELETE CASCADE,"
        " train_id        INTEGER NOT NULL REFERENCES trains(id),"
        " travel_date     TEXT NOT NULL,"
        " class_code      TEXT NOT NULL,"
        " fare            INTEGER NOT NULL CHECK (fare > 0),"
        " passenger_count INTEGER NOT NULL CHECK (passenger_count > 0),"
        " status          TEXT NOT NULL DEFAULT 'CONFIRMED'"
        "                 CHECK (status IN ('CONFIRMED','CANCELLED')),"
        " created_at      TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now')),"
        " cancelled_at    TEXT,"
        " FOREIGN KEY (train_id, class_code)"
        "   REFERENCES train_classes(train_id, class_code)"
        ")");
    db.execute("CREATE INDEX IF NOT EXISTS idx_bookings_user ON bookings (user_id)");
    if (!hasColumn(db, "bookings", "cancelled_at"))
        db.execute("ALTER TABLE bookings ADD COLUMN cancelled_at TEXT");
    db.execute(
        "CREATE TABLE IF NOT EXISTS passengers ("
        " id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        " booking_id  INTEGER NOT NULL REFERENCES bookings(id) ON DELETE CASCADE,"
        " name        TEXT NOT NULL,"
        " age         INTEGER NOT NULL CHECK (age BETWEEN 1 AND 120),"
        " gender      TEXT NOT NULL CHECK (gender IN ('M','F','O')),"
        " seat_number TEXT NOT NULL"
        ")");
    db.execute("CREATE INDEX IF NOT EXISTS idx_passengers_booking ON passengers (booking_id)");
    db.execute(
        "CREATE TABLE IF NOT EXISTS seat_reservations ("
        " id          INTEGER PRIMARY KEY AUTOINCREMENT,"
        " train_id    INTEGER NOT NULL,"
        " travel_date TEXT NOT NULL,"
        " class_code  TEXT NOT NULL,"
        " seats       INTEGER NOT NULL CHECK (seats > 0),"
        " booking_id  INTEGER REFERENCES bookings(id) ON DELETE CASCADE,"
        " FOREIGN KEY (train_id, class_code)"
        "   REFERENCES train_classes(train_id, class_code) ON DELETE CASCADE"
        ")");
    if (!hasColumn(db, "seat_reservations", "booking_id"))
        db.execute("ALTER TABLE seat_reservations ADD COLUMN booking_id INTEGER REFERENCES bookings(id)");
}

}  // namespace rtrs
