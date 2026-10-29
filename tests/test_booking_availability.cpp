#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;
using nlohmann::json;

namespace {

long long addSmallTrain(TestServer& s) {
    return test::addTrain(s.db(), "93003", "Small Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, 4}, {"3A", "AC 3 Tier", 700, 10}});
}

httplib::Result preview(TestServer& s, const std::string& token, const json& body) {
    return s.client().Post("/api/bookings/preview", test::bearer(token), body.dump(), "application/json");
}

}  // namespace

TEST(BookingAvailabilityApi, AllowsBookingExactlyTheRemainingSeats) {
    TestServer s;
    long long trainId = addSmallTrain(s);
    std::string token = test::registerAndLogin(s);
    test::reserveSeats(s.db(), trainId, "2026-11-01", "SL", 1);
    auto body = test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar", "Meera Nair"});

    auto res = preview(s, token, body);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_EQ(json::parse(res->body)["preview"]["availableSeats"], 3);
}

TEST(BookingAvailabilityApi, RejectsBookingMoreThanAvailable) {
    TestServer s;
    long long trainId = addSmallTrain(s);
    std::string token = test::registerAndLogin(s);
    test::reserveSeats(s.db(), trainId, "2026-11-01", "SL", 2);
    auto body = test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar", "Meera Nair"});

    auto res = preview(s, token, body);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 409);
    auto response = json::parse(res->body);
    EXPECT_EQ(response["error"], "Not enough seats are available.");
    EXPECT_EQ(response["fields"]["passengers"], "Only 2 seat(s) available in Sleeper.");
}

TEST(BookingAvailabilityApi, SoldOutClassIsRejected) {
    TestServer s;
    long long trainId = addSmallTrain(s);
    std::string token = test::registerAndLogin(s);
    test::reserveSeats(s.db(), trainId, "2026-11-01", "3A", 10);

    auto res = preview(s, token, test::bookingBody(trainId, "3A"));

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 409);
    EXPECT_EQ(json::parse(res->body)["fields"]["passengers"], "Only 0 seat(s) available in AC 3 Tier.");
}

TEST(BookingAvailabilityApi, AvailabilityIsPerDate) {
    TestServer s;
    long long trainId = addSmallTrain(s);
    std::string token = test::registerAndLogin(s);
    test::reserveSeats(s.db(), trainId, "2026-11-01", "SL", 4);

    auto otherDate = preview(s, token, test::bookingBody(trainId, "SL", {"Asha Rao"}, "2026-11-02"));
    auto sameDate = preview(s, token, test::bookingBody(trainId, "SL", {"Asha Rao"}, "2026-11-01"));

    ASSERT_TRUE(otherDate);
    ASSERT_TRUE(sameDate);
    EXPECT_EQ(otherDate->status, 200);
    EXPECT_EQ(sameDate->status, 409);
}

TEST(BookingAvailabilityApi, ValidationErrorsTakePrecedenceOverSoldOut) {
    TestServer s;
    long long trainId = addSmallTrain(s);
    std::string token = test::registerAndLogin(s);
    test::reserveSeats(s.db(), trainId, "2026-11-01", "SL", 4);
    auto body = test::bookingBody(trainId, "SL", {"A"});

    auto res = preview(s, token, body);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    auto fields = json::parse(res->body)["fields"];
    EXPECT_TRUE(fields.count("passengers.0.name"));
    EXPECT_FALSE(fields.count("passengers"));
}
