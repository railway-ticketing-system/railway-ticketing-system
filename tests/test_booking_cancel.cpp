#include <gtest/gtest.h>

#include <ctime>
#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;
using nlohmann::json;

namespace {

long long addBookingTrain(TestServer& s) {
    return test::addTrain(s.db(), "93008", "Cancel Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, 10}});
}

std::string dateAfter(int days) {
    std::time_t when = std::time(nullptr) + static_cast<std::time_t>(days) * 86400;
    std::tm utc = *std::gmtime(&when);
    char buffer[11];
    std::strftime(buffer, sizeof buffer, "%Y-%m-%d", &utc);
    return buffer;
}

json createBooking(TestServer& s, const std::string& token, long long trainId, int daysAhead) {
    auto res = s.client().Post("/api/bookings", test::bearer(token),
                               test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar"},
                                                 dateAfter(daysAhead)).dump(),
                               "application/json");
    if (!res || res->status != 201) return json::object();
    return json::parse(res->body)["booking"];
}

httplib::Result cancelBooking(TestServer& s, const std::string& token, long long id) {
    return s.client().Post("/api/bookings/" + std::to_string(id) + "/cancel", test::bearer(token),
                           "", "application/json");
}

}  // namespace

TEST(BookingCancelApi, RequiresAuthentication) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 3);

    auto res = s.client().Post("/api/bookings/" + std::to_string(booking["id"].get<long long>()) + "/cancel",
                               "", "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST(BookingCancelApi, CancelsEligibleBooking) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 3);

    auto res = cancelBooking(s, token, booking["id"]);

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto cancelled = json::parse(res->body)["booking"];
    EXPECT_EQ(cancelled["status"], "CANCELLED");
    EXPECT_FALSE(cancelled["cancelledAt"].get<std::string>().empty());

    auto ticket = s.client().Get("/api/bookings/" + booking["pnr"].get<std::string>(), test::bearer(token));
    ASSERT_TRUE(ticket);
    ASSERT_EQ(ticket->status, 200);
    EXPECT_EQ(json::parse(ticket->body)["booking"]["status"], "CANCELLED");
}

TEST(BookingCancelApi, RejectsCancellationAfterCutoff) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 0);

    auto res = cancelBooking(s, token, booking["id"]);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 409);
    EXPECT_NE(json::parse(res->body)["fields"]["booking"].get<std::string>().find("24 hours"),
              std::string::npos);
    auto ticket = s.client().Get("/api/bookings/" + booking["pnr"].get<std::string>(), test::bearer(token));
    ASSERT_TRUE(ticket);
    EXPECT_EQ(json::parse(ticket->body)["booking"]["status"], "CONFIRMED");
}

TEST(BookingCancelApi, RejectsSecondCancellation) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 3);
    ASSERT_EQ(cancelBooking(s, token, booking["id"])->status, 200);

    auto res = cancelBooking(s, token, booking["id"]);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 409);
    EXPECT_EQ(json::parse(res->body)["fields"]["booking"], "This booking is already cancelled.");
}

TEST(BookingCancelApi, UnknownBookingReturnsNotFound) {
    TestServer s;
    std::string token = test::registerAndLogin(s);

    auto res = cancelBooking(s, token, 9999);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
    EXPECT_EQ(json::parse(res->body)["error"], "Booking not found.");
}

TEST(BookingCancelApi, HonoursConfiguredCutoff) {
    TestServer s(CancellationPolicy{48, 90});
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 1);

    auto res = cancelBooking(s, token, booking["id"]);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 409);
    EXPECT_NE(json::parse(res->body)["fields"]["booking"].get<std::string>().find("48 hours"),
              std::string::npos);
}
