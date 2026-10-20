#pragma once
#include <optional>
#include <string>
#include <vector>

#include "db/Database.h"
#include "util/Validation.h"

namespace rtrs {

struct TrainSummary {
    long long id = 0;
    std::string number, name, source, destination, departure, arrival;
    int durationMinutes = 0;
    int fromFare = 0;
};

struct SearchQuery {
    std::string source, destination, date;
    std::string travelClass;
    std::string sort = "departure";
    std::string order = "asc";
    std::string maxFare;
    std::string departureFrom, departureTo;
};

struct SearchResult {
    std::vector<TrainSummary> trains;
    FieldErrors errors;
    bool ok() const { return errors.empty(); }
};

struct Stop {
    std::string station, arrival, departure;
    int dayOffset = 0;
};

struct TrainSchedule {
    long long id = 0;
    std::string number, name;
    std::vector<Stop> stops;
};

class TrainService {
public:
    explicit TrainService(Database& db) : db_(db) {}

    SearchResult search(const SearchQuery& query);
    std::optional<TrainSchedule> findById(long long id);

private:
    Database& db_;
};

}  // namespace rtrs
