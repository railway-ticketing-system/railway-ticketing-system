#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;
using nlohmann::json;

namespace {

long long addBookingTrain(TestServer& s) {
    return test::addTrain(s.db(), "93006", "Ticket Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, 10}});
}

json createBooking(TestServer& s, const std::string& token, long long trainId) {
    auto res = s.client().Post("/api/bookings", test::bearer(token),
                               test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar"}).dump(),
                               "application/json");
    if (!res || res->status != 201) return json::object();
    return json::parse(res->body)["booking"];
}

httplib::Result getTicket(TestServer& s, const std::string& token, const std::string& pnr) {
    return s.client().Get("/api/bookings/" + pnr, test::bearer(token));
}

}  // namespace

TEST(BookingTicketApi, OwnerCanViewTicket) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto created = createBooking(s, token, trainId);

    auto res = getTicket(s, token, created["pnr"]);

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto booking = json::parse(res->body)["booking"];
    EXPECT_EQ(booking["pnr"], created["pnr"]);
    EXPECT_EQ(booking["status"], "CONFIRMED");
    EXPECT_EQ(booking["trainNumber"], "93006");
    EXPECT_EQ(booking["trainName"], "Ticket Express");
    EXPECT_EQ(booking["date"], "2026-11-01");
    EXPECT_EQ(booking["className"], "Sleeper");
    EXPECT_EQ(booking["totalFare"], 500);
    ASSERT_EQ(booking["passengers"].size(), 2u);
    EXPECT_EQ(booking["passengers"][0]["name"], "Asha Rao");
    EXPECT_EQ(booking["passengers"][0]["age"], 30);
    EXPECT_EQ(booking["passengers"][0]["gender"], "M");
    EXPECT_EQ(booking["passengers"][0]["seatNumber"], "SL-1");
    EXPECT_EQ(booking["passengers"][1]["seatNumber"], "SL-2");
}

TEST(BookingTicketApi, OtherUsersCannotViewTicket) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string owner = test::registerAndLogin(s, "owner@example.com");
    std::string other = test::registerAndLogin(s, "other@example.com", "Ravi Kumar");
    auto created = createBooking(s, owner, trainId);

    auto res = getTicket(s, other, created["pnr"]);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
    EXPECT_EQ(json::parse(res->body)["error"], "Booking not found.");
}

TEST(BookingTicketApi, UnknownPnrReturnsNotFound) {
    TestServer s;
    std::string token = test::registerAndLogin(s);

    auto res = getTicket(s, token, "1234567890");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
    EXPECT_EQ(json::parse(res->body)["error"], "Booking not found.");
}

TEST(BookingTicketApi, RequiresAuthentication) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto created = createBooking(s, token, trainId);

    auto res = s.client().Get("/api/bookings/" + created["pnr"].get<std::string>());

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}
