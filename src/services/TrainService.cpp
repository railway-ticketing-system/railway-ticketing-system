#include "services/TrainService.h"

#include <algorithm>
#include <cctype>

namespace rtrs {

namespace {

std::string lower(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    return value;
}

int toMinutes(const std::string& time) {
    return std::stoi(time.substr(0, 2)) * 60 + std::stoi(time.substr(3, 2));
}

FieldErrors validateQuery(const SearchQuery& query) {
    FieldErrors errors;
    std::string source = trim(query.source);
    std::string destination = trim(query.destination);

    if (!isValidStation(source)) errors["source"] = "Enter a valid source station.";
    if (!isValidStation(destination)) errors["destination"] = "Enter a valid destination station.";
    if (errors.empty() && lower(source) == lower(destination))
        errors["destination"] = "Destination must be different from source.";
    if (!isValidDate(trim(query.date))) errors["date"] = "Enter a valid travel date (YYYY-MM-DD).";

    std::string travelClass = trim(query.travelClass);
    if (!travelClass.empty() && !isValidClassCode(travelClass))
        errors["class"] = "Choose a valid travel class.";

    std::string maxFare = trim(query.maxFare);
    if (!maxFare.empty()) {
        bool digits = std::all_of(maxFare.begin(), maxFare.end(),
                                  [](unsigned char c) { return std::isdigit(c) != 0; });
        if (!digits || maxFare.size() > 7 || std::stoi(maxFare) <= 0)
            errors["maxFare"] = "Enter a maximum fare in rupees.";
    }

    std::string departureFrom = trim(query.departureFrom);
    std::string departureTo = trim(query.departureTo);
    if (!departureFrom.empty() && !isValidTime(departureFrom))
        errors["departureFrom"] = "Enter a valid time (HH:MM).";
    if (!departureTo.empty() && !isValidTime(departureTo))
        errors["departureTo"] = "Enter a valid time (HH:MM).";
    if (isValidTime(departureFrom) && isValidTime(departureTo) && departureFrom > departureTo)
        errors["departureTo"] = "End time must be after the start time.";

    std::string sort = trim(query.sort);
    if (!sort.empty() && !isValidSortField(sort))
        errors["sort"] = "Sort by departure, price or duration.";
    std::string order = trim(query.order);
    if (!order.empty() && order != "asc" && order != "desc")
        errors["order"] = "Order must be asc or desc.";
    return errors;
}

}  // namespace

SearchResult TrainService::search(const SearchQuery& query) {
    SearchResult result;
    result.errors = validateQuery(query);
    if (!result.ok()) return result;

    std::string source = trim(query.source);
    std::string destination = trim(query.destination);
    std::string travelClass = trim(query.travelClass);
    std::string maxFare = trim(query.maxFare);
    std::string departureFrom = trim(query.departureFrom);
    std::string departureTo = trim(query.departureTo);

    std::string sql =
        "SELECT t.id, t.number, t.name, src.station, dst.station, src.departure_time, dst.arrival_time,"
        " src.day_offset, dst.day_offset,"
        " (SELECT MIN(tc.fare) FROM train_classes tc WHERE tc.train_id = t.id"
        "  AND (? = '' OR tc.class_code = ?)) AS min_fare"
        " FROM trains t"
        " JOIN train_stops src ON src.train_id = t.id AND lower(src.station) = lower(?)"
        "  AND COALESCE(src.departure_time, '') <> ''"
        " JOIN train_stops dst ON dst.train_id = t.id AND lower(dst.station) = lower(?)"
        "  AND dst.seq > src.seq AND COALESCE(dst.arrival_time, '') <> ''"
        " WHERE 1 = 1";

    std::vector<Database::Param> params{travelClass, travelClass, source, destination};
    if (!travelClass.empty()) {
        sql += " AND EXISTS (SELECT 1 FROM train_classes c1 WHERE c1.train_id = t.id AND c1.class_code = ?)";
        params.push_back(travelClass);
    }
    if (!maxFare.empty()) {
        sql += " AND EXISTS (SELECT 1 FROM train_classes c2 WHERE c2.train_id = t.id AND c2.fare <= ?"
               "  AND (? = '' OR c2.class_code = ?))";
        params.push_back(maxFare);
        params.push_back(travelClass);
        params.push_back(travelClass);
    }
    if (!departureFrom.empty()) {
        sql += " AND src.departure_time >= ?";
        params.push_back(departureFrom);
    }
    if (!departureTo.empty()) {
        sql += " AND src.departure_time <= ?";
        params.push_back(departureTo);
    }

    auto rows = db_.query(sql, params);

    for (const auto& row : rows) {
        TrainSummary train;
        train.id = std::stoll(row[0]);
        train.number = row[1];
        train.name = row[2];
        train.source = row[3];
        train.destination = row[4];
        train.departure = row[5];
        train.arrival = row[6];

        int sourceDay = std::stoi(row[7]);
        int destinationDay = std::stoi(row[8]);
        train.durationMinutes =
            (destinationDay - sourceDay) * 1440 + toMinutes(train.arrival) - toMinutes(train.departure);
        train.fromFare = row[9].empty() ? 0 : std::stoi(row[9]);
        if (train.durationMinutes <= 0) continue;
        result.trains.push_back(std::move(train));
    }

    std::string sort = trim(query.sort).empty() ? "departure" : trim(query.sort);
    std::string order = trim(query.order).empty() ? "asc" : trim(query.order);
    std::stable_sort(result.trains.begin(), result.trains.end(),
                     [&sort](const TrainSummary& a, const TrainSummary& b) {
                         if (sort == "price" && a.fromFare != b.fromFare) return a.fromFare < b.fromFare;
                         if (sort == "duration" && a.durationMinutes != b.durationMinutes)
                             return a.durationMinutes < b.durationMinutes;
                         if (a.departure != b.departure) return a.departure < b.departure;
                         return a.name < b.name;
                     });
    if (order == "desc") std::reverse(result.trains.begin(), result.trains.end());
    return result;
}

std::optional<TrainSchedule> TrainService::findById(long long id) {
    auto rows = db_.query("SELECT id, number, name FROM trains WHERE id = ?", {std::to_string(id)});
    if (rows.empty()) return std::nullopt;

    TrainSchedule schedule;
    schedule.id = std::stoll(rows[0][0]);
    schedule.number = rows[0][1];
    schedule.name = rows[0][2];

    auto stops = db_.query(
        "SELECT station, COALESCE(arrival_time, ''), COALESCE(departure_time, ''), day_offset"
        " FROM train_stops WHERE train_id = ? ORDER BY seq",
        {std::to_string(id)});
    for (const auto& row : stops) {
        Stop stop;
        stop.station = row[0];
        stop.arrival = row[1];
        stop.departure = row[2];
        stop.dayOffset = std::stoi(row[3]);
        schedule.stops.push_back(std::move(stop));
    }
    return schedule;
}

std::vector<AvailabilityEntry> TrainService::availability(long long id, const std::string& date) {
    auto rows = db_.query(
        "SELECT c.class_code, c.class_name, c.total_seats, c.fare,"
        " COALESCE((SELECT SUM(r.seats) FROM seat_reservations r"
        "  WHERE r.train_id = c.train_id AND r.class_code = c.class_code AND r.travel_date = ?), 0)"
        " FROM train_classes c WHERE c.train_id = ? ORDER BY c.fare",
        {date, std::to_string(id)});

    std::vector<AvailabilityEntry> entries;
    for (const auto& row : rows) {
        AvailabilityEntry entry;
        entry.code = row[0];
        entry.name = row[1];
        entry.totalSeats = std::stoi(row[2]);
        entry.fare = std::stoi(row[3]);
        entry.bookedSeats = std::stoi(row[4]);
        entry.availableSeats = std::max(0, entry.totalSeats - entry.bookedSeats);
        entries.push_back(std::move(entry));
    }
    return entries;
}

std::vector<TrainClass> TrainService::fares(long long id) {
    auto rows = db_.query(
        "SELECT class_code, class_name, fare, total_seats FROM train_classes"
        " WHERE train_id = ? ORDER BY fare",
        {std::to_string(id)});

    std::vector<TrainClass> classes;
    for (const auto& row : rows) {
        TrainClass cls;
        cls.code = row[0];
        cls.name = row[1];
        cls.fare = std::stoi(row[2]);
        cls.totalSeats = std::stoi(row[3]);
        classes.push_back(std::move(cls));
    }
    return classes;
}

}  // namespace rtrs
