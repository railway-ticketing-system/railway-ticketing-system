#include "services/BookingService.h"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <ctime>
#include <random>

namespace rtrs {

namespace {

std::string trimValue(const std::string& value) { return trim(value); }

long long epochAt(const std::string& date, const std::string& time) {
    int year = 0, month = 0, day = 0, hour = 0, minute = 0;
    if (std::sscanf(date.c_str(), "%d-%d-%d", &year, &month, &day) != 3) return -1;
    if (!time.empty() && std::sscanf(time.c_str(), "%d:%d", &hour, &minute) != 2) return -1;
    std::tm tm{};
    tm.tm_year = year - 1900;
    tm.tm_mon = month - 1;
    tm.tm_mday = day;
    tm.tm_hour = hour;
    tm.tm_min = minute;
    return static_cast<long long>(timegm(&tm));
}

FieldErrors validatePassengers(const std::vector<PassengerInput>& passengers) {
    FieldErrors errors;
    for (size_t i = 0; i < passengers.size(); ++i) {
        const auto& passenger = passengers[i];
        const std::string prefix = "passengers." + std::to_string(i) + ".";

        if (!isValidName(trimValue(passenger.name)))
            errors[prefix + "name"] = "Enter the passenger's name.";

        std::string age = trimValue(passenger.age);
        bool digits = !age.empty() && age.size() <= 3 &&
                      std::all_of(age.begin(), age.end(),
                                  [](unsigned char c) { return std::isdigit(c) != 0; });
        if (!digits || std::stoi(age) < 1 || std::stoi(age) > 120)
            errors[prefix + "age"] = "Age must be between 1 and 120.";

        std::string gender = trimValue(passenger.gender);
        if (gender != "M" && gender != "F" && gender != "O")
            errors[prefix + "gender"] = "Select the passenger's gender.";
    }
    return errors;
}

struct SoldOut {};
struct PnrUnavailable {};
struct BookingNotFound {};
struct NotCancellable {
    std::string reason;
};

std::string randomPnr() {
    static thread_local std::mt19937_64 rng(std::random_device{}());
    std::uniform_int_distribution<long long> digits(1000000000LL, 9999999999LL);
    return std::to_string(digits(rng));
}

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
    auto passengerErrors = validatePassengers(input.passengers);
    result.errors.insert(passengerErrors.begin(), passengerErrors.end());

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

    if (result.errors.empty() && result.preview.availableSeats < result.preview.passengerCount) {
        result.conflict = "sold_out";
        result.errors["passengers"] = "Only " + std::to_string(result.preview.availableSeats) +
                                      " seat(s) available in " + result.preview.className + ".";
    }

    result.ok = result.errors.empty() && result.conflict.empty();
    return result;
}

ConfirmResult BookingService::confirm(const BookingInput& input) {
    ConfirmResult result;
    auto checked = preview(input);
    result.errors = checked.errors;
    result.conflict = checked.conflict;
    if (!checked.ok) return result;

    const auto& draft = checked.preview;
    long long bookingId = 0;

    try {
        db_.transaction([&] {
            auto rows = db_.query(
                "SELECT c.total_seats,"
                " COALESCE((SELECT SUM(r.seats) FROM seat_reservations r"
                "  WHERE r.train_id = c.train_id AND r.class_code = c.class_code"
                "  AND r.travel_date = ?), 0)"
                " FROM train_classes c WHERE c.train_id = ? AND c.class_code = ?",
                {draft.travelDate, std::to_string(input.trainId), draft.classCode});
            if (rows.empty()) throw SoldOut{};

            int totalSeats = std::stoi(rows[0][0]);
            int bookedSeats = std::stoi(rows[0][1]);
            int count = draft.passengerCount;
            if (bookedSeats + count > totalSeats) throw SoldOut{};

            std::string pnr;
            for (int attempt = 0; attempt < 20 && pnr.empty(); ++attempt) {
                std::string candidate = randomPnr();
                if (db_.query("SELECT 1 FROM bookings WHERE pnr = ?", {candidate}).empty())
                    pnr = candidate;
            }
            if (pnr.empty()) throw PnrUnavailable{};

            db_.execute(
                "INSERT INTO bookings"
                " (pnr, user_id, train_id, travel_date, class_code, fare, passenger_count)"
                " VALUES (?, ?, ?, ?, ?, ?, ?)",
                {pnr, std::to_string(input.userId), std::to_string(input.trainId), draft.travelDate,
                 draft.classCode, std::to_string(draft.farePerPassenger), std::to_string(count)});
            bookingId = db_.lastInsertId();

            for (int i = 0; i < count; ++i) {
                const auto& passenger = input.passengers[i];
                std::string seat = draft.classCode + "-" + std::to_string(bookedSeats + i + 1);
                db_.execute(
                    "INSERT INTO passengers (booking_id, name, age, gender, seat_number)"
                    " VALUES (?, ?, ?, ?, ?)",
                    {std::to_string(bookingId), trimValue(passenger.name),
                     std::to_string(std::stoi(trimValue(passenger.age))), trimValue(passenger.gender), seat});
            }

            db_.execute(
                "INSERT INTO seat_reservations (train_id, travel_date, class_code, seats, booking_id)"
                " VALUES (?, ?, ?, ?, ?)",
                {std::to_string(input.trainId), draft.travelDate, draft.classCode, std::to_string(count),
                 std::to_string(bookingId)});
        });
    } catch (const SoldOut&) {
        result.conflict = "sold_out";
        result.errors["passengers"] = "Not enough seats are available.";
        return result;
    } catch (const PnrUnavailable&) {
        result.conflict = "pnr_unavailable";
        result.errors["booking"] = "Could not generate a PNR. Please try again.";
        return result;
    }

    auto booking = loadBooking(bookingId);
    if (!booking) {
        result.errors["booking"] = "Booking could not be loaded.";
        return result;
    }
    result.booking = *booking;
    result.ok = true;
    return result;
}

std::optional<Booking> BookingService::findById(long long id) { return loadBooking(id); }

bool BookingService::cancellable(const Booking& booking, std::string& reason) const {
    if (booking.status != "CONFIRMED") {
        reason = "This booking is already cancelled.";
        return false;
    }
    auto rows = db_.query(
        "SELECT COALESCE(departure_time, ''), day_offset FROM train_stops"
        " WHERE train_id = ? ORDER BY seq LIMIT 1",
        {std::to_string(booking.trainId)});
    if (rows.empty() || rows[0][0].empty()) {
        reason = "Cancellation is not available for this train.";
        return false;
    }
    long long departure = epochAt(booking.travelDate, rows[0][0]);
    if (departure < 0) {
        reason = "Cancellation is not available for this booking.";
        return false;
    }
    departure += std::stoll(rows[0][1]) * 86400LL;
    long long now = static_cast<long long>(std::time(nullptr));
    if (departure - now < static_cast<long long>(policy_.cutoffHours) * 3600) {
        reason = "Cancellation closes " + std::to_string(policy_.cutoffHours) + " hours before departure.";
        return false;
    }
    return true;
}

CancelResult BookingService::cancel(long long id) {
    CancelResult result;
    std::string reason;
    try {
        db_.transaction([&] {
            auto rows = db_.query("SELECT status FROM bookings WHERE id = ?", {std::to_string(id)});
            if (rows.empty()) throw BookingNotFound{};
            if (rows[0][0] != "CONFIRMED") throw NotCancellable{"This booking is already cancelled."};
            auto booking = loadBooking(id);
            if (!booking || !cancellable(*booking, reason)) throw NotCancellable{reason};
            db_.execute(
                "UPDATE bookings SET status = 'CANCELLED',"
                " cancelled_at = strftime('%Y-%m-%dT%H:%M:%SZ','now') WHERE id = ?",
                {std::to_string(id)});
        });
    } catch (const BookingNotFound&) {
        result.conflict = "not_found";
        return result;
    } catch (const NotCancellable& e) {
        result.conflict = "not_cancellable";
        result.errors["booking"] = e.reason;
        return result;
    }

    auto booking = loadBooking(id);
    if (!booking) {
        result.errors["booking"] = "Booking could not be loaded.";
        return result;
    }
    result.booking = *booking;
    result.ok = true;
    return result;
}

std::optional<Booking> BookingService::findByPnr(const std::string& pnr) {
    auto rows = db_.query("SELECT id FROM bookings WHERE pnr = ?", {pnr});
    if (rows.empty()) return std::nullopt;
    return loadBooking(std::stoll(rows[0][0]));
}

std::vector<Booking> BookingService::listForUser(long long userId) {
    std::vector<Booking> bookings;
    auto rows = db_.query("SELECT id FROM bookings WHERE user_id = ? ORDER BY id DESC",
                          {std::to_string(userId)});
    for (const auto& row : rows) {
        auto booking = loadBooking(std::stoll(row[0]));
        if (booking) bookings.push_back(std::move(*booking));
    }
    return bookings;
}

std::optional<Booking> BookingService::loadBooking(long long id) {
    auto rows = db_.query(
        "SELECT b.id, b.pnr, b.user_id, b.train_id, b.travel_date, b.class_code, b.fare,"
        " b.passenger_count, b.status, b.created_at, b.cancelled_at, t.number, t.name, c.class_name"
        " FROM bookings b"
        " JOIN trains t ON t.id = b.train_id"
        " JOIN train_classes c ON c.train_id = b.train_id AND c.class_code = b.class_code"
        " WHERE b.id = ?",
        {std::to_string(id)});
    if (rows.empty()) return std::nullopt;

    Booking booking;
    booking.id = std::stoll(rows[0][0]);
    booking.pnr = rows[0][1];
    booking.userId = std::stoll(rows[0][2]);
    booking.trainId = std::stoll(rows[0][3]);
    booking.travelDate = rows[0][4];
    booking.classCode = rows[0][5];
    booking.farePerPassenger = std::stoi(rows[0][6]);
    booking.passengerCount = std::stoi(rows[0][7]);
    booking.status = rows[0][8];
    booking.createdAt = rows[0][9];
    booking.cancelledAt = rows[0][10];
    booking.trainNumber = rows[0][11];
    booking.trainName = rows[0][12];
    booking.className = rows[0][13];
    booking.totalFare = booking.farePerPassenger * booking.passengerCount;

    auto passengers = db_.query(
        "SELECT name, age, gender, seat_number FROM passengers WHERE booking_id = ? ORDER BY id",
        {std::to_string(id)});
    for (const auto& row : passengers) {
        Passenger passenger;
        passenger.name = row[0];
        passenger.age = std::stoi(row[1]);
        passenger.gender = row[2];
        passenger.seatNumber = row[3];
        booking.passengers.push_back(std::move(passenger));
    }
    return booking;
}

}  // namespace rtrs
