#include "services/AdminTrainService.h"

#include <algorithm>
#include <cctype>
#include <set>

namespace rtrs {
namespace {

std::string lower(std::string text) {
    std::transform(text.begin(), text.end(), text.begin(), [](unsigned char c) { return std::tolower(c); });
    return text;
}

void normalize(TrainInput& input) {
    input.number = trim(input.number);
    input.name = trim(input.name);
    for (auto& stop : input.stops) {
        stop.station = trim(stop.station);
        stop.arrival = trim(stop.arrival);
        stop.departure = trim(stop.departure);
    }
    for (auto& cls : input.classes) {
        cls.code = trim(cls.code);
        cls.name = trim(cls.name);
    }
}

bool validLabel(const std::string& text) {
    return text.size() >= 2 && text.size() <= 100 &&
        std::any_of(text.begin(), text.end(), [](unsigned char c) { return std::isalpha(c); }) &&
        std::all_of(text.begin(), text.end(), [](unsigned char c) {
            return std::isalnum(c) || c == ' ' || c == '.' || c == '-' || c == '\'' || c == '&' || c == '(' || c == ')';
        });
}

int minutes(const std::string& time) {
    return std::stoi(time.substr(0, 2)) * 60 + std::stoi(time.substr(3, 2));
}

FieldErrors validate(const TrainInput& input) {
    FieldErrors errors;
    if (input.number.size() != 5 || !std::all_of(input.number.begin(), input.number.end(),
            [](unsigned char c) { return std::isdigit(c); }))
        errors["number"] = "Enter a five-digit train number.";
    if (!validLabel(input.name)) errors["name"] = "Enter a train name (2-100 characters).";
    if (input.stops.size() < 2 || input.stops.size() > 100)
        errors["stops"] = "Provide between 2 and 100 stops in journey order.";
    std::set<std::string> stations;
    int previousDeparture = -1;
    for (size_t i = 0; i < input.stops.size(); ++i) {
        const auto& stop = input.stops[i];
        auto prefix = "stops." + std::to_string(i) + ".";
        if (!isValidStation(stop.station)) errors[prefix + "station"] = "Enter a valid station name.";
        else if (!stations.insert(lower(stop.station)).second)
            errors[prefix + "station"] = "Stations must be unique within a route.";
        bool validDay = stop.dayOffset >= 0 && stop.dayOffset <= 30;
        if (!validDay) errors[prefix + "dayOffset"] = "Day offset must be between 0 and 30.";
        bool arrivalValid = isValidTime(stop.arrival);
        bool departureValid = isValidTime(stop.departure);
        if (i == 0) {
            if (!stop.arrival.empty()) errors[prefix + "arrival"] = "The first stop has no arrival time.";
        } else if (!arrivalValid) errors[prefix + "arrival"] = "Enter arrival time as HH:MM.";
        if (i + 1 == input.stops.size()) {
            if (!stop.departure.empty()) errors[prefix + "departure"] = "The last stop has no departure time.";
        } else if (!departureValid) errors[prefix + "departure"] = "Enter departure time as HH:MM.";
        if (!validDay) continue;
        int arrival = arrivalValid ? stop.dayOffset * 1440 + minutes(stop.arrival) : -1;
        int departure = departureValid ? stop.dayOffset * 1440 + minutes(stop.departure) : -1;
        if (i > 0 && arrival >= 0 && previousDeparture >= 0 && arrival <= previousDeparture)
            errors[prefix + "arrival"] = "Arrival must follow the previous stop's departure; check day offsets.";
        if (arrival >= 0 && departure >= 0 && departure < arrival)
            errors[prefix + "departure"] = "Departure must not precede arrival.";
        previousDeparture = departure;
    }
    if (input.classes.empty() || input.classes.size() > 6)
        errors["classes"] = "Provide between 1 and 6 travel classes.";
    std::set<std::string> codes;
    for (size_t i = 0; i < input.classes.size(); ++i) {
        const auto& cls = input.classes[i];
        auto prefix = "classes." + std::to_string(i) + ".";
        if (!isValidClassCode(cls.code) || !codes.insert(cls.code).second)
            errors[prefix + "code"] = "Choose a valid, unique travel class.";
        if (!validLabel(cls.name)) errors[prefix + "name"] = "Enter a class name (2-100 characters).";
        if (cls.fare < 1 || cls.fare > 1000000) errors[prefix + "fare"] = "Fare must be between 1 and 1000000 rupees.";
        if (cls.totalSeats < 1 || cls.totalSeats > 10000)
            errors[prefix + "totalSeats"] = "Seat capacity must be between 1 and 10000.";
    }
    return errors;
}

}

std::optional<ManagedTrain> AdminTrainService::findById(long long id) {
    auto schedule = trains_.findById(id);
    if (!schedule) return std::nullopt;
    ManagedTrain train;
    train.id = id;
    train.details.number = schedule->number;
    train.details.name = schedule->name;
    train.details.stops = schedule->stops;
    train.details.classes = trains_.fares(id);
    return train;
}

std::vector<ManagedTrain> AdminTrainService::list() {
    std::vector<ManagedTrain> result;
    for (const auto& row : db_.query("SELECT id FROM trains ORDER BY number")) {
        auto train = findById(std::stoll(row[0]));
        if (train) result.push_back(*train);
    }
    return result;
}

void AdminTrainService::insertStops(long long id, const TrainInput& input) {
    for (size_t i = 0; i < input.stops.size(); ++i) {
        const auto& stop = input.stops[i];
        db_.execute("INSERT INTO train_stops (train_id, station, arrival_time, departure_time, day_offset, seq)"
                    " VALUES (?, ?, ?, ?, ?, ?)",
                    {std::to_string(id), stop.station, stop.arrival, stop.departure,
                     std::to_string(stop.dayOffset), std::to_string(i + 1)});
    }
}

TrainWriteResult AdminTrainService::create(TrainInput input) {
    normalize(input);
    TrainWriteResult result;
    result.errors = validate(input);
    if (!result.errors.empty()) return result;
    long long id = 0;
    db_.transaction([&] {
        if (!db_.query("SELECT id FROM trains WHERE number = ?", {input.number}).empty()) {
            result.status = 409;
            result.errors["number"] = "This train number is already registered.";
            return;
        }
        db_.execute("INSERT INTO trains (number, name) VALUES (?, ?)", {input.number, input.name});
        id = db_.lastInsertId();
        insertStops(id, input);
        for (const auto& cls : input.classes)
            db_.execute("INSERT INTO train_classes (train_id, class_code, class_name, fare, total_seats)"
                        " VALUES (?, ?, ?, ?, ?)", {std::to_string(id), cls.code, cls.name,
                            std::to_string(cls.fare), std::to_string(cls.totalSeats)});
    });
    if (!id) return result;
    result.train = *findById(id);
    result.status = 201;
    return result;
}

TrainWriteResult AdminTrainService::update(long long id, TrainInput input) {
    normalize(input);
    TrainWriteResult result;
    result.errors = validate(input);
    if (!result.errors.empty()) return result;
    db_.transaction([&] {
        auto existing = findById(id);
        if (!existing) {
            result.status = 404;
            return;
        }
        result.status = 409;
        if (!db_.query("SELECT id FROM trains WHERE number = ? AND id <> ?",
                        {input.number, std::to_string(id)}).empty()) {
            result.errors["number"] = "This train number is already registered.";
            return;
        }
        std::set<std::string> codes;
        for (size_t i = 0; i < input.classes.size(); ++i) {
            const auto& cls = input.classes[i];
            codes.insert(cls.code);
            auto reserved = db_.query(
                "SELECT COALESCE(MAX(booked), 0) FROM (SELECT SUM(seats) AS booked FROM seat_reservations"
                " WHERE train_id = ? AND class_code = ? GROUP BY travel_date)", {std::to_string(id), cls.code});
            auto highestSeat = db_.query(
                "SELECT COALESCE(MAX(CAST(substr(p.seat_number, instr(p.seat_number, '-') + 1) AS INTEGER)), 0)"
                " FROM passengers p JOIN bookings b ON b.id = p.booking_id"
                " WHERE b.train_id = ? AND b.class_code = ? AND b.status = 'CONFIRMED'", {std::to_string(id), cls.code});
            int minimum = std::max(std::stoi(reserved[0][0]), std::stoi(highestSeat[0][0]));
            if (cls.totalSeats < minimum)
                result.errors["classes." + std::to_string(i) + ".totalSeats"] =
                    "Capacity must accommodate all reserved seats and assigned seat numbers (minimum " +
                    std::to_string(minimum) + ").";
        }
        for (const auto& cls : existing->details.classes) {
            if (codes.count(cls.code)) continue;
            if (!db_.query("SELECT id FROM bookings WHERE train_id = ? AND class_code = ? LIMIT 1",
                           {std::to_string(id), cls.code}).empty() ||
                !db_.query("SELECT id FROM seat_reservations WHERE train_id = ? AND class_code = ? LIMIT 1",
                           {std::to_string(id), cls.code}).empty())
                result.errors["classes"] = "Classes referenced by bookings or reservations cannot be removed.";
        }
        if (!result.errors.empty()) return;
        db_.execute("UPDATE trains SET number = ?, name = ? WHERE id = ?",
                    {input.number, input.name, std::to_string(id)});
        db_.execute("DELETE FROM train_stops WHERE train_id = ?", {std::to_string(id)});
        insertStops(id, input);
        for (const auto& cls : existing->details.classes)
            if (!codes.count(cls.code))
                db_.execute("DELETE FROM train_classes WHERE train_id = ? AND class_code = ?",
                            {std::to_string(id), cls.code});
        for (const auto& cls : input.classes)
            db_.execute("INSERT INTO train_classes (train_id, class_code, class_name, fare, total_seats)"
                        " VALUES (?, ?, ?, ?, ?) ON CONFLICT(train_id, class_code) DO UPDATE SET"
                        " class_name = excluded.class_name, fare = excluded.fare, total_seats = excluded.total_seats",
                        {std::to_string(id), cls.code, cls.name, std::to_string(cls.fare), std::to_string(cls.totalSeats)});
        result.train = *findById(id);
        result.status = 200;
    });
    return result;
}

}
