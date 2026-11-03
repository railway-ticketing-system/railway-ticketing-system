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
        " name   TEXT NOT NULL,"
        " active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1))"
        ")");
    if (!hasColumn(db, "trains", "active"))
        db.execute("ALTER TABLE trains ADD COLUMN active INTEGER NOT NULL DEFAULT 1 CHECK (active IN (0, 1))");
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
    // RTS-NF-001: search matches stations case-insensitively, so index the expression.
    db.execute("CREATE INDEX IF NOT EXISTS idx_train_stops_station_nocase ON train_stops (lower(station))");
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
        " refund_amount   INTEGER NOT NULL DEFAULT 0 CHECK (refund_amount >= 0),"
        " FOREIGN KEY (train_id, class_code)"
        "   REFERENCES train_classes(train_id, class_code)"
        ")");
    db.execute("CREATE INDEX IF NOT EXISTS idx_bookings_user ON bookings (user_id)");
    if (!hasColumn(db, "bookings", "cancelled_at"))
        db.execute("ALTER TABLE bookings ADD COLUMN cancelled_at TEXT");
    if (!hasColumn(db, "bookings", "refund_amount"))
        db.execute("ALTER TABLE bookings ADD COLUMN refund_amount INTEGER NOT NULL DEFAULT 0 CHECK (refund_amount >= 0)");
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
    // RTS-NF-002: a seat can belong to at most one confirmed booking for a
    // train/date/class, even if a future process bypasses the service layer.
    db.execute(
        "CREATE TRIGGER IF NOT EXISTS trg_passengers_unique_seat"
        " BEFORE INSERT ON passengers"
        " WHEN EXISTS ("
        "  SELECT 1 FROM passengers p"
        "  JOIN bookings b ON b.id = p.booking_id"
        "  JOIN bookings nb ON nb.id = NEW.booking_id"
        "  WHERE b.status = 'CONFIRMED'"
        "    AND b.train_id = nb.train_id"
        "    AND b.travel_date = nb.travel_date"
        "    AND b.class_code = nb.class_code"
        "    AND p.seat_number = NEW.seat_number"
        " )"
        " BEGIN SELECT RAISE(ABORT, 'seat already allocated'); END");
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
    // RTS-NF-001: indexes for search, availability and booking inventory lookups.
    db.execute("CREATE INDEX IF NOT EXISTS idx_train_classes_train ON train_classes (train_id, fare)");
    db.execute(
        "CREATE INDEX IF NOT EXISTS idx_seat_reservations_inventory"
        " ON seat_reservations (train_id, travel_date, class_code)");
    db.execute(
        "CREATE INDEX IF NOT EXISTS idx_bookings_inventory"
        " ON bookings (train_id, travel_date, class_code, status)");
    // RTS-F-018 / RTS-NF-008: append-only audit trail for bookings and cancellations.
    db.execute(
        "CREATE TABLE IF NOT EXISTS audit_events ("
        " id            INTEGER PRIMARY KEY AUTOINCREMENT,"
        " event_type    TEXT NOT NULL"
        "               CHECK (event_type IN ('BOOKING_CREATED','BOOKING_CANCELLED')),"
        " user_id       INTEGER NOT NULL REFERENCES users(id),"
        " booking_id    INTEGER NOT NULL REFERENCES bookings(id),"
        " train_id      INTEGER NOT NULL REFERENCES trains(id),"
        " pnr           TEXT NOT NULL,"
        " refund_amount INTEGER NOT NULL DEFAULT 0 CHECK (refund_amount >= 0),"
        " created_at    TEXT NOT NULL DEFAULT (strftime('%Y-%m-%dT%H:%M:%SZ','now'))"
        ")");
    db.execute("CREATE INDEX IF NOT EXISTS idx_audit_events_booking ON audit_events (booking_id)");
    db.execute("CREATE INDEX IF NOT EXISTS idx_audit_events_created ON audit_events (created_at)");
}

}  // namespace rtrs
