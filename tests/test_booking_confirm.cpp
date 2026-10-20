#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;
using nlohmann::json;

namespace {

long long addBookingTrain(TestServer& s) {
    return test::addTrain(s.db(), "93004", "Confirm Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, 4}, {"3A", "AC 3 Tier", 700, 10}});
}

httplib::Result confirm(TestServer& s, const std::string& token, const json& body) {
    return s.client().Post("/api/bookings", test::bearer(token), body.dump(), "application/json");
}

int bookingCount(TestServer& s) {
    return std::stoi(s.db().query("SELECT COUNT(*) FROM bookings")[0][0]);
}

}  // namespace

TEST(BookingConfirmApi, RequiresAuthentication) {
    TestServer s;
    long long trainId = addBookingTrain(s);

    auto res = s.client().Post("/api/bookings", test::bookingBody(trainId).dump(), "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
    EXPECT_EQ(bookingCount(s), 0);
}

TEST(BookingConfirmApi, CreatesConfirmedBookingWithSeatsAndPnr) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);

    auto res = confirm(s, token, test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar"}));

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 201);
    auto booking = json::parse(res->body)["booking"];
    EXPECT_EQ(booking["status"], "CONFIRMED");
    EXPECT_EQ(booking["trainNumber"], "93004");
    EXPECT_EQ(booking["className"], "Sleeper");
    EXPECT_EQ(booking["date"], "2026-11-01");
    EXPECT_EQ(booking["passengerCount"], 2);
    EXPECT_EQ(booking["totalFare"], 500);

    std::string pnr = booking["pnr"];
    ASSERT_EQ(pnr.size(), 10u);
    EXPECT_TRUE(std::all_of(pnr.begin(), pnr.end(), [](unsigned char c) { return std::isdigit(c) != 0; }));

    ASSERT_EQ(booking["passengers"].size(), 2u);
    EXPECT_EQ(booking["passengers"][0]["name"], "Asha Rao");
    EXPECT_EQ(booking["passengers"][0]["seatNumber"], "SL-1");
    EXPECT_EQ(booking["passengers"][1]["seatNumber"], "SL-2");
}

TEST(BookingConfirmApi, ReservesSeatsAndReducesAvailability) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar"});

    auto res = confirm(s, token, body);
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 201);
    long long bookingId = json::parse(res->body)["booking"]["id"];

    auto reservation = s.db().query(
        "SELECT seats, booking_id FROM seat_reservations"
        " WHERE train_id = ? AND travel_date = ? AND class_code = 'SL'",
        {std::to_string(trainId), "2026-11-01"});
    ASSERT_EQ(reservation.size(), 1u);
    EXPECT_EQ(reservation[0][0], "2");
    EXPECT_EQ(std::stoll(reservation[0][1]), bookingId);

    auto availability = s.client().Get("/api/trains/" + std::to_string(trainId) +
                                       "/availability?date=2026-11-01");
    ASSERT_TRUE(availability);
    ASSERT_EQ(availability->status, 200);
    for (const auto& entry : json::parse(availability->body)["classes"])
        if (entry["code"] == "SL") EXPECT_EQ(entry["availableSeats"], 2);
}

TEST(BookingConfirmApi, RejectsInvalidPassengersWithoutCreatingBooking) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId, "SL", {"A"});

    auto res = confirm(s, token, body);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_TRUE(json::parse(res->body)["fields"].count("passengers.0.name"));
    EXPECT_EQ(bookingCount(s), 0);
}

TEST(BookingConfirmApi, RejectsWhenSeatsRanOutAfterPreview) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar", "Meera Nair"});

    auto preview = s.client().Post("/api/bookings/preview", test::bearer(token), body.dump(),
                                   "application/json");
    ASSERT_TRUE(preview);
    ASSERT_EQ(preview->status, 200);

    test::reserveSeats(s.db(), trainId, "2026-11-01", "SL", 2);

    auto res = confirm(s, token, body);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 409);
    EXPECT_EQ(json::parse(res->body)["fields"]["passengers"], "Only 2 seat(s) available in Sleeper.");
    EXPECT_EQ(bookingCount(s), 0);
}

TEST(BookingConfirmApi, RejectsSoldOutClass) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    test::reserveSeats(s.db(), trainId, "2026-11-01", "SL", 4);

    auto res = confirm(s, token, test::bookingBody(trainId));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 409);
    EXPECT_EQ(bookingCount(s), 0);
}

TEST(BookingConfirmApi, GivesEachBookingADistinctPnrAndSeats) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);

    auto first = confirm(s, token, test::bookingBody(trainId, "SL", {"Asha Rao"}));
    auto second = confirm(s, token, test::bookingBody(trainId, "SL", {"Ravi Kumar"}));

    ASSERT_TRUE(first);
    ASSERT_TRUE(second);
    ASSERT_EQ(first->status, 201);
    ASSERT_EQ(second->status, 201);
    auto a = json::parse(first->body)["booking"];
    auto b = json::parse(second->body)["booking"];
    EXPECT_NE(a["pnr"], b["pnr"]);
    EXPECT_EQ(a["passengers"][0]["seatNumber"], "SL-1");
    EXPECT_EQ(b["passengers"][0]["seatNumber"], "SL-2");
    EXPECT_EQ(bookingCount(s), 2);
}
