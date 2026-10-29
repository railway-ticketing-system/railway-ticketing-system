#pragma once
#include <optional>
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

struct Passenger {
    std::string name, gender, seatNumber;
    int age = 0;
};

struct Booking {
    long long id = 0, userId = 0, trainId = 0;
    std::string pnr, status, trainNumber, trainName, travelDate, classCode, className, createdAt;
    std::string cancelledAt;
    int farePerPassenger = 0, totalFare = 0, passengerCount = 0;
    std::vector<Passenger> passengers;
};

struct ConfirmResult {
    bool ok = false;
    FieldErrors errors;
    std::string conflict;
    Booking booking;
};

struct CancellationPolicy {
    int cutoffHours = 24;
    int refundPercent = 90;
};

struct CancelResult {
    bool ok = false;
    FieldErrors errors;
    std::string conflict;
    Booking booking;
};

class BookingService {
public:
    BookingService(Database& db, TrainService& trains, CancellationPolicy policy = {})
        : db_(db), trains_(trains), policy_(policy) {}

    PreviewResult preview(const BookingInput& input);
    ConfirmResult confirm(const BookingInput& input);
    std::optional<Booking> findByPnr(const std::string& pnr);
    std::optional<Booking> findById(long long id);
    std::vector<Booking> listForUser(long long userId);
    CancelResult cancel(long long id);

    const CancellationPolicy& policy() const { return policy_; }

private:
    std::optional<Booking> loadBooking(long long id);
    bool cancellable(const Booking& booking, std::string& reason) const;

    Database& db_;
    TrainService& trains_;
    CancellationPolicy policy_;
};

}  // namespace rtrs
