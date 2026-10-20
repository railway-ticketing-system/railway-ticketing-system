#include "services/BookingService.h"

#include <algorithm>
#include <cctype>

namespace rtrs {

namespace {

std::string trimValue(const std::string& value) { return trim(value); }

FieldErrors validateInput(const BookingInput& input) {
    FieldErrors errors;
    if (input.trainId <= 0) errors["trainId"] = "Choose a train.";
    if (!isValidDate(trimValue(input.travelDate)))
        errors["date"] = "Enter a valid travel date (YYYY-MM-DD).";
    if (!isValidClassCode(trimValue(input.classCode)))
        errors["class"] = "Choose a valid travel class.";
    if (input.passengers.empty())
        errors["passengers"] = "Add at least one passenger.";
    else if (input.passengers.size() > 6)
        errors["passengers"] = "A booking can have at most 6 passengers.";
    return errors;
}

}  // namespace

PreviewResult BookingService::preview(const BookingInput& input) {
    PreviewResult result;
    result.errors = validateInput(input);

    if (input.trainId <= 0) return result;

    auto rows = db_.query(
        "SELECT t.number, t.name, c.class_name, c.fare"
        " FROM trains t JOIN train_classes c ON c.train_id = t.id"
        " WHERE t.id = ? AND c.class_code = ?",
        {std::to_string(input.trainId), trimValue(input.classCode)});

    if (rows.empty()) {
        if (!trains_.findById(input.trainId)) {
            result.errors["trainId"] = "Train not found.";
        } else {
            result.errors["class"] = "This class is not available on the selected train.";
        }
        return result;
    }

    result.preview.trainId = input.trainId;
    result.preview.trainNumber = rows[0][0];
    result.preview.trainName = rows[0][1];
    result.preview.classCode = trimValue(input.classCode);
    result.preview.className = rows[0][2];
    result.preview.farePerPassenger = std::stoi(rows[0][3]);
    result.preview.travelDate = trimValue(input.travelDate);
    result.preview.passengerCount = static_cast<int>(input.passengers.size());
    result.preview.totalFare = result.preview.farePerPassenger * result.preview.passengerCount;

    for (const auto& entry : trains_.availability(input.trainId, result.preview.travelDate)) {
        if (entry.code == result.preview.classCode) {
            result.preview.availableSeats = entry.availableSeats;
            break;
        }
    }

    result.ok = result.errors.empty();
    return result;
}

}  // namespace rtrs
