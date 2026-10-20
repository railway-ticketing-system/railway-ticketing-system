#pragma once
#include <string>
#include <vector>

#include "db/Database.h"

namespace rtrs::test {

struct StopIn {
    std::string station, arrival, departure;
    int day = 0;
};

struct ClassIn {
    std::string code, name;
    int fare = 0;
    int seats = 100;
};

inline long long addTrain(Database& db, const std::string& number, const std::string& name,
                          const std::vector<StopIn>& stops, const std::vector<ClassIn>& classes) {
    db.execute("INSERT INTO trains (number, name) VALUES (?, ?)", {number, name});
    long long id = db.lastInsertId();
    int seq = 1;
    for (const auto& stop : stops) {
        db.execute(
            "INSERT INTO train_stops (train_id, station, arrival_time, departure_time, day_offset, seq)"
            " VALUES (?, ?, ?, ?, ?, ?)",
            {std::to_string(id), stop.station, stop.arrival, stop.departure, std::to_string(stop.day),
             std::to_string(seq++)});
    }
    for (const auto& cls : classes) {
        db.execute(
            "INSERT INTO train_classes (train_id, class_code, class_name, fare, total_seats)"
            " VALUES (?, ?, ?, ?, ?)",
            {std::to_string(id), cls.code, cls.name, std::to_string(cls.fare), std::to_string(cls.seats)});
    }
    return id;
}

inline void reserveSeats(Database& db, long long trainId, const std::string& date, const std::string& code,
                         int seats) {
    db.execute(
        "INSERT INTO seat_reservations (train_id, travel_date, class_code, seats) VALUES (?, ?, ?, ?)",
        {std::to_string(trainId), date, code, std::to_string(seats)});
}

}  // namespace rtrs::test
