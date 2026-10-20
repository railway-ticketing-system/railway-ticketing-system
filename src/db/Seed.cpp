#include "db/Seed.h"

#include <vector>

namespace rtrs {

namespace {

struct StopSeed {
    const char* station;
    const char* arrival;
    const char* departure;
    int dayOffset;
};

struct ClassSeed {
    const char* code;
    const char* name;
    int fare;
    int totalSeats;
};

struct TrainSeed {
    const char* number;
    const char* name;
    std::vector<StopSeed> stops;
    std::vector<ClassSeed> classes;
};

const std::vector<TrainSeed>& demoTrains() {
    static const std::vector<TrainSeed> trains = {
        {"12127", "Intercity Express",
         {{"Pune Jn", "", "06:15", 0},
          {"Daund Jn", "07:05", "07:10", 0},
          {"Solapur Jn", "10:20", "10:25", 0},
          {"Hyderabad Deccan", "13:40", "", 0}},
         {{"SL", "Sleeper", 250, 210}, {"3A", "AC 3 Tier", 650, 90}, {"2A", "AC 2 Tier", 920, 45}}},
        {"11007", "Deccan Express",
         {{"Pune Jn", "", "07:15", 0},
          {"Lonavala", "08:05", "08:07", 0},
          {"Mumbai CSMT", "10:45", "", 0}},
         {{"SL", "Sleeper", 190, 300}, {"CC", "AC Chair Car", 420, 120}, {"2A", "AC 2 Tier", 780, 40}}},
        {"12027", "Shatabdi Express",
         {{"Pune Jn", "", "17:30", 0}, {"Mumbai CSMT", "20:40", "", 0}},
         {{"CC", "AC Chair Car", 520, 400}, {"EC", "Executive Chair Car", 980, 100}}},
        {"12627", "Karnataka Express",
         {{"Bengaluru City", "", "19:20", 0},
          {"Tumakuru", "20:35", "20:37", 0},
          {"Davangere", "23:50", "23:55", 0},
          {"Pune Jn", "08:10", "", 1}},
         {{"SL", "Sleeper", 380, 250}, {"3A", "AC 3 Tier", 980, 120}, {"2A", "AC 2 Tier", 1380, 60}}},
        {"12951", "Mumbai Rajdhani",
         {{"Mumbai CSMT", "", "17:00", 0},
          {"Surat", "19:55", "20:00", 0},
          {"Vadodara Jn", "21:35", "21:45", 0},
          {"Delhi", "08:30", "", 1}},
         {{"3A", "AC 3 Tier", 1450, 300}, {"2A", "AC 2 Tier", 2050, 150}, {"1A", "AC First Class", 3200, 40}}},
    };
    return trains;
}

}  // namespace

void seedDemoData(Database& db) {
    auto rows = db.query("SELECT COUNT(*) FROM trains");
    if (!rows.empty() && rows[0][0] != "0") return;

    db.transaction([&] {
        for (const auto& train : demoTrains()) {
            db.execute("INSERT INTO trains (number, name) VALUES (?, ?)", {train.number, train.name});
            long long trainId = db.lastInsertId();
            int seq = 1;
            for (const auto& stop : train.stops) {
                db.execute(
                    "INSERT INTO train_stops (train_id, station, arrival_time, departure_time, day_offset, seq)"
                    " VALUES (?, ?, ?, ?, ?, ?)",
                    {std::to_string(trainId), stop.station, stop.arrival, stop.departure,
                     std::to_string(stop.dayOffset), std::to_string(seq++)});
            }
            for (const auto& cls : train.classes) {
                db.execute(
                    "INSERT INTO train_classes (train_id, class_code, class_name, fare, total_seats)"
                    " VALUES (?, ?, ?, ?, ?)",
                    {std::to_string(trainId), cls.code, cls.name, std::to_string(cls.fare),
                     std::to_string(cls.totalSeats)});
            }
        }
    });
}

}  // namespace rtrs
