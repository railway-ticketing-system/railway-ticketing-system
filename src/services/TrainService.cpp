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
    return errors;
}

}  // namespace

SearchResult TrainService::search(const SearchQuery& query) {
    SearchResult result;
    result.errors = validateQuery(query);
    if (!result.ok()) return result;

    std::string source = trim(query.source);
    std::string destination = trim(query.destination);

    auto rows = db_.query(
        "SELECT t.id, t.number, t.name, src.station, dst.station, src.departure_time, dst.arrival_time,"
        " src.day_offset, dst.day_offset,"
        " (SELECT MIN(tc.fare) FROM train_classes tc WHERE tc.train_id = t.id) AS min_fare"
        " FROM trains t"
        " JOIN train_stops src ON src.train_id = t.id AND lower(src.station) = lower(?)"
        "  AND COALESCE(src.departure_time, '') <> ''"
        " JOIN train_stops dst ON dst.train_id = t.id AND lower(dst.station) = lower(?)"
        "  AND dst.seq > src.seq AND COALESCE(dst.arrival_time, '') <> ''",
        {source, destination});

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

    std::stable_sort(result.trains.begin(), result.trains.end(),
                     [](const TrainSummary& a, const TrainSummary& b) {
                         if (a.departure != b.departure) return a.departure < b.departure;
                         return a.name < b.name;
                     });
    return result;
}

}  // namespace rtrs
