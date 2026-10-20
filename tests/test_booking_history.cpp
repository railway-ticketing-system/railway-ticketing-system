#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;
using nlohmann::json;

namespace {

long long addBookingTrain(TestServer& s) {
    return test::addTrain(s.db(), "93007", "History Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, 20}});
}

json createBooking(TestServer& s, const std::string& token, long long trainId,
                   const std::vector<std::string>& names = {"Asha Rao"}) {
    auto res = s.client().Post("/api/bookings", test::bearer(token),
                               test::bookingBody(trainId, "SL", names).dump(), "application/json");
    if (!res || res->status != 201) return json::object();
    return json::parse(res->body)["booking"];
}

json history(TestServer& s, const std::string& token) {
    auto res = s.client().Get("/api/bookings", test::bearer(token));
    if (!res || res->status != 200) return json::object();
    return json::parse(res->body);
}

}  // namespace

TEST(BookingHistoryApi, RequiresAuthentication) {
    TestServer s;

    auto res = s.client().Get("/api/bookings");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST(BookingHistoryApi, ReturnsEmptyListWhenNoBookings) {
    TestServer s;
    std::string token = test::registerAndLogin(s);

    auto body = history(s, token);

    ASSERT_TRUE(body.contains("bookings"));
    EXPECT_TRUE(body["bookings"].is_array());
    EXPECT_TRUE(body["bookings"].empty());
}

TEST(BookingHistoryApi, ReturnsBookingsNewestFirst) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto first = createBooking(s, token, trainId);
    auto second = createBooking(s, token, trainId);

    auto body = history(s, token);

    ASSERT_EQ(body["bookings"].size(), 2u);
    EXPECT_EQ(body["bookings"][0]["pnr"], second["pnr"]);
    EXPECT_EQ(body["bookings"][1]["pnr"], first["pnr"]);
}

TEST(BookingHistoryApi, IncludesTicketDetails) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    createBooking(s, token, trainId, {"Asha Rao", "Ravi Kumar"});

    auto booking = history(s, token)["bookings"][0];

    EXPECT_EQ(booking["trainNumber"], "93007");
    EXPECT_EQ(booking["trainName"], "History Express");
    EXPECT_EQ(booking["status"], "CONFIRMED");
    EXPECT_EQ(booking["date"], "2026-11-01");
    EXPECT_EQ(booking["class"], "SL");
    EXPECT_EQ(booking["passengerCount"], 2);
    EXPECT_EQ(booking["totalFare"], 500);
    ASSERT_EQ(booking["passengers"].size(), 2u);
    EXPECT_EQ(booking["passengers"][0]["seatNumber"], "SL-1");
    EXPECT_EQ(booking["passengers"][1]["name"], "Ravi Kumar");
}

TEST(BookingHistoryApi, DoesNotIncludeOtherUsersBookings) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string owner = test::registerAndLogin(s, "owner@example.com");
    std::string other = test::registerAndLogin(s, "other@example.com", "Ravi Kumar");
    createBooking(s, owner, trainId);
    createBooking(s, other, trainId, {"Meera Nair"});

    auto body = history(s, owner);

    ASSERT_EQ(body["bookings"].size(), 1u);
    EXPECT_EQ(body["bookings"][0]["passengers"][0]["name"], "Asha Rao");
}
