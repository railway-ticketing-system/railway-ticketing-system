#pragma once
#include <string>
#include <vector>

#include "db/Database.h"
#include "services/TrainService.h"
#include "util/Validation.h"

namespace rtrs {

struct PassengerInput {
    std::string name, age, gender;
};

struct BookingInput {
    long long userId = 0;
    long long trainId = 0;
    std::string travelDate, classCode;
    std::vector<PassengerInput> passengers;
};

struct BookingPreview {
    long long trainId = 0;
    std::string trainNumber, trainName, travelDate, classCode, className;
    int farePerPassenger = 0, passengerCount = 0, totalFare = 0, availableSeats = 0;
};

struct PreviewResult {
    bool ok = false;
    FieldErrors errors;
    std::string conflict;
    BookingPreview preview;
};

class BookingService {
public:
    BookingService(Database& db, TrainService& trains) : db_(db), trains_(trains) {}

    PreviewResult preview(const BookingInput& input);

private:
    Database& db_;
    TrainService& trains_;
};

}  // namespace rtrs
