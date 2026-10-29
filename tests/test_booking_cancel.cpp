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

TEST(BookingRefundApi, QuotePreviewsRefundWithoutChangingBooking) {
    TestServer s;
    auto trainId = addBookingTrain(s);
    auto token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 3);
    auto path = "/api/bookings/" + std::to_string(booking["id"].get<long long>());
    auto res = s.client().Get(path + "/cancellation", test::bearer(token));
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto quote = json::parse(res->body)["cancellation"];
    EXPECT_EQ(quote["eligible"], true);
    EXPECT_EQ(quote["refundAmount"], 450);
    EXPECT_EQ(quote["refundPercent"], 90);
    EXPECT_EQ(quote["cutoffHours"], 24);
    auto row = s.db().query("SELECT status, refund_amount FROM bookings WHERE id = ?",
                            {std::to_string(booking["id"].get<long long>())});
    EXPECT_EQ(row[0][0], "CONFIRMED");
    EXPECT_EQ(row[0][1], "0");
    auto cancelled = cancelBooking(s, token, booking["id"]);
    ASSERT_TRUE(cancelled);
    ASSERT_EQ(cancelled->status, 200);
    EXPECT_EQ(json::parse(cancelled->body)["booking"]["refundAmount"], quote["refundAmount"]);
    auto stored = s.db().query("SELECT refund_amount FROM bookings WHERE id = ?",
                               {std::to_string(booking["id"].get<long long>())});
    EXPECT_EQ(stored[0][0], "450");
}

TEST(BookingRefundApi, RefundUsesConfiguredPercentageAndRoundsDown) {
    TestServer s(CancellationPolicy{24, 33});
    auto trainId = addBookingTrain(s);
    auto token = test::registerAndLogin(s);
    auto res = s.client().Post("/api/bookings", test::bearer(token),
        test::bookingBody(trainId, "SL", {"Asha Rao"}, dateAfter(3)).dump(), "application/json");
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 201);
    auto booking = json::parse(res->body)["booking"];
    auto cancelled = cancelBooking(s, token, booking["id"]);
    ASSERT_TRUE(cancelled);
    ASSERT_EQ(cancelled->status, 200);
    EXPECT_EQ(json::parse(cancelled->body)["booking"]["refundAmount"], 82);
}

TEST(BookingRefundApi, IneligibleQuoteHasReasonAndNoRefund) {
    TestServer s;
    auto trainId = addBookingTrain(s);
    auto token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 0);
    auto res = s.client().Get("/api/bookings/" + std::to_string(booking["id"].get<long long>()) +
                              "/cancellation", test::bearer(token));
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto quote = json::parse(res->body)["cancellation"];
    EXPECT_EQ(quote["eligible"], false);
    EXPECT_EQ(quote["refundAmount"], 0);
    EXPECT_FALSE(quote["reason"].get<std::string>().empty());
}
