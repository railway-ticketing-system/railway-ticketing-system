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

TEST(BookingRestoreApi, CancellationRestoresOnlyItsOwnSeatsExactlyOnce) {
    TestServer s;
    auto trainId = addBookingTrain(s);
    auto token = test::registerAndLogin(s);
    auto first = createBooking(s, token, trainId, 3);
    auto second = createBooking(s, token, trainId, 3);
    test::reserveSeats(s.db(), trainId, dateAfter(4), "SL", 3);
    TrainService trains(s.db());
    EXPECT_EQ(trains.availability(trainId, dateAfter(3))[0].availableSeats, 6);
    auto cancelled = cancelBooking(s, token, first["id"]);
    ASSERT_TRUE(cancelled);
    ASSERT_EQ(cancelled->status, 200);
    EXPECT_EQ(trains.availability(trainId, dateAfter(3))[0].availableSeats, 8);
    EXPECT_EQ(trains.availability(trainId, dateAfter(4))[0].availableSeats, 7);
    auto repeated = cancelBooking(s, token, first["id"]);
    ASSERT_TRUE(repeated);
    EXPECT_EQ(repeated->status, 409);
    EXPECT_EQ(trains.availability(trainId, dateAfter(3))[0].availableSeats, 8);
    auto reservations = s.db().query("SELECT booking_id, seats FROM seat_reservations WHERE booking_id IS NOT NULL");
    ASSERT_EQ(reservations.size(), 1u);
    EXPECT_EQ(reservations[0][0], std::to_string(second["id"].get<long long>()));
    EXPECT_EQ(reservations[0][1], "2");
}

TEST(BookingRestoreApi, RebookingReusesFreedSeatsWithoutDuplicatingActiveSeats) {
    TestServer s;
    auto trainId = addBookingTrain(s);
    auto token = test::registerAndLogin(s);
    auto first = createBooking(s, token, trainId, 3);
    auto second = createBooking(s, token, trainId, 3);
    auto cancelled = cancelBooking(s, token, first["id"]);
    ASSERT_TRUE(cancelled);
    ASSERT_EQ(cancelled->status, 200);
    auto replacement = createBooking(s, token, trainId, 3);
    ASSERT_FALSE(replacement.empty());
    EXPECT_EQ(replacement["passengers"][0]["seatNumber"], "SL-1");
    EXPECT_EQ(replacement["passengers"][1]["seatNumber"], "SL-2");
    EXPECT_EQ(second["passengers"][0]["seatNumber"], "SL-3");
    auto counts = s.db().query(
        "SELECT COUNT(*), COUNT(DISTINCT p.seat_number) FROM passengers p"
        " JOIN bookings b ON b.id = p.booking_id WHERE b.status = 'CONFIRMED'");
    EXPECT_EQ(counts[0][0], "4");
    EXPECT_EQ(counts[0][1], "4");
}

TEST(BookingRestoreApi, InventoryFailureRollsBackCancellationAndRefund) {
    TestServer s;
    auto trainId = addBookingTrain(s);
    auto token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 3);
    s.db().execute("CREATE TRIGGER reject_release BEFORE DELETE ON seat_reservations"
                   " BEGIN SELECT RAISE(ABORT, 'inventory unavailable'); END");
    auto res = cancelBooking(s, token, booking["id"]);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 500);
    auto rows = s.db().query("SELECT status, refund_amount, cancelled_at FROM bookings");
    EXPECT_EQ(rows[0][0], "CONFIRMED");
    EXPECT_EQ(rows[0][1], "0");
    EXPECT_EQ(rows[0][2], "");
    TrainService trains(s.db());
    EXPECT_EQ(trains.availability(trainId, dateAfter(3))[0].availableSeats, 8);
}

TEST(BookingCancelSecurityApi, AnotherUserCannotPreviewOrCancelEvenWithSpoofedOwner) {
    TestServer s;
    auto trainId = addBookingTrain(s);
    auto owner = test::registerAndLogin(s, "owner@example.com");
    auto attacker = test::registerAndLogin(s, "attacker@example.com");
    auto booking = createBooking(s, owner, trainId, 3);
    auto path = "/api/bookings/" + std::to_string(booking["id"].get<long long>());
    auto quote = s.client().Get(path + "/cancellation", test::bearer(attacker));
    ASSERT_TRUE(quote);
    EXPECT_EQ(quote->status, 404);
    auto cancel = s.client().Post(path + "/cancel", test::bearer(attacker),
                                  R"({"userId":1,"refundAmount":500})", "application/json");
    ASSERT_TRUE(cancel);
    EXPECT_EQ(cancel->status, 404);
    auto put = s.client().Put(path + "/cancel", test::bearer(attacker), "", "application/json");
    ASSERT_TRUE(put);
    EXPECT_EQ(put->status, 404);
    auto rows = s.db().query("SELECT status, refund_amount, cancelled_at FROM bookings");
    EXPECT_EQ(rows[0][0], "CONFIRMED");
    EXPECT_EQ(rows[0][1], "0");
    EXPECT_EQ(rows[0][2], "");
    TrainService trains(s.db());
    EXPECT_EQ(trains.availability(trainId, dateAfter(3))[0].availableSeats, 8);
    auto allowed = s.client().Put(path + "/cancel", test::bearer(owner), "", "application/json");
    ASSERT_TRUE(allowed);
    EXPECT_EQ(allowed->status, 200);
    EXPECT_EQ(json::parse(allowed->body)["booking"]["refundAmount"], 450);
}

TEST(BookingCancelSecurityApi, QuoteAndCancellationRejectMissingInvalidAndRevokedSessions) {
    TestServer s;
    auto trainId = addBookingTrain(s);
    auto token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 3);
    auto path = "/api/bookings/" + std::to_string(booking["id"].get<long long>());
    auto missing = s.client().Get(path + "/cancellation");
    ASSERT_TRUE(missing);
    EXPECT_EQ(missing->status, 401);
    auto invalid = cancelBooking(s, "invalid", booking["id"]);
    ASSERT_TRUE(invalid);
    EXPECT_EQ(invalid->status, 401);
    auto logout = s.client().Post("/api/auth/logout", test::bearer(token), "", "application/json");
    ASSERT_TRUE(logout);
    auto revoked = cancelBooking(s, token, booking["id"]);
    ASSERT_TRUE(revoked);
    EXPECT_EQ(revoked->status, 401);
    EXPECT_EQ(s.db().query("SELECT status FROM bookings")[0][0], "CONFIRMED");
}

TEST(BookingCancelSecurityApi, ServiceEnforcesOwnerInsideCancellationTransaction) {
    TestServer s;
    auto trainId = addBookingTrain(s);
    auto token = test::registerAndLogin(s);
    auto booking = createBooking(s, token, trainId, 3);
    TrainService trains(s.db());
    BookingService bookings(s.db(), trains);
    auto result = bookings.cancel(booking["id"], 9999);
    EXPECT_FALSE(result.ok);
    EXPECT_EQ(result.conflict, "not_found");
    EXPECT_EQ(s.db().query("SELECT status FROM bookings")[0][0], "CONFIRMED");
}
