#include <gtest/gtest.h>

#include <ctime>
#include <nlohmann/json.hpp>

#include "AdminHelpers.h"
#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;
using nlohmann::json;

namespace {

std::string dateAfter(int days) {
    std::time_t when = std::time(nullptr) + static_cast<std::time_t>(days) * 86400;
    std::tm utc = *std::gmtime(&when);
    char buffer[11];
    std::strftime(buffer, sizeof buffer, "%Y-%m-%d", &utc);
    return buffer;
}

long long addAuditTrain(TestServer& s, const std::string& number = "95001", int seats = 10) {
    return test::addTrain(s.db(), number, "Audit Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, seats}});
}

json createBooking(TestServer& s, const std::string& token, long long trainId,
                   const std::vector<std::string>& names = {"Asha Rao"}) {
    auto res = s.client().Post("/api/bookings", test::bearer(token),
                               test::bookingBody(trainId, "SL", names, dateAfter(3)).dump(),
                               "application/json");
    if (!res || res->status != 201) return json::object();
    return json::parse(res->body)["booking"];
}

}  // namespace

TEST(AuditApi, RecordsTimestampedBookingAndCancellationEvents) {
    TestServer s;
    long long trainId = addAuditTrain(s, "95001");
    std::string token = test::registerAndLogin(s, "auditor@example.com", "Asha Rao");
    auto booking = createBooking(s, token, trainId, {"Asha Rao", "Ravi Kumar"});
    ASSERT_FALSE(booking.empty());

    auto created = s.db().query(
        "SELECT event_type, user_id, booking_id, train_id, pnr, refund_amount, created_at FROM audit_events");
    ASSERT_EQ(created.size(), 1u);
    EXPECT_EQ(created[0][0], "BOOKING_CREATED");
    EXPECT_EQ(created[0][1], "1");
    EXPECT_EQ(created[0][2], std::to_string(booking["id"].get<long long>()));
    EXPECT_EQ(created[0][3], std::to_string(trainId));
    EXPECT_EQ(created[0][4], booking["pnr"].get<std::string>());
    EXPECT_EQ(created[0][5], "0");
    EXPECT_EQ(created[0][6].size(), 20u);
    EXPECT_EQ(created[0][6].back(), 'Z');

    auto cancel = s.client().Post("/api/bookings/" + std::to_string(booking["id"].get<long long>()) + "/cancel",
                                  test::bearer(token), "", "application/json");
    ASSERT_TRUE(cancel);
    ASSERT_EQ(cancel->status, 200);

    auto events = s.db().query(
        "SELECT event_type, booking_id, refund_amount, created_at FROM audit_events ORDER BY id");
    ASSERT_EQ(events.size(), 2u);
    EXPECT_EQ(events[1][0], "BOOKING_CANCELLED");
    EXPECT_EQ(events[1][1], std::to_string(booking["id"].get<long long>()));
    EXPECT_EQ(events[1][2], "450");
    EXPECT_EQ(events[1][3].size(), 20u);
}

TEST(AuditApi, FailedBookingDoesNotLeaveAnAuditEvent) {
    TestServer s;
    long long trainId = addAuditTrain(s, "95002", 1);
    std::string first = test::registerAndLogin(s, "first@example.com", "Asha Rao");
    std::string second = test::registerAndLogin(s, "second@example.com", "Ravi Kumar");
    ASSERT_FALSE(createBooking(s, first, trainId).empty());

    auto rejected = s.client().Post("/api/bookings", test::bearer(second),
                                    test::bookingBody(trainId, "SL", {"Ravi Kumar"}, dateAfter(3)).dump(),
                                    "application/json");
    ASSERT_TRUE(rejected);
    EXPECT_EQ(rejected->status, 409);
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM audit_events")[0][0], "1");
}

TEST(AuditApi, AdministratorCanReadTheAuditTrail) {
    TestServer s;
    long long trainId = addAuditTrain(s, "95003");
    std::string passenger = test::registerAndLogin(s, "passenger@example.com", "Asha Rao");
    auto booking = createBooking(s, passenger, trainId);
    ASSERT_FALSE(booking.empty());
    std::string admin = test::adminLogin(s);

    auto res = s.client().Get("/api/admin/audit", test::bearer(admin));
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto events = json::parse(res->body)["events"];
    ASSERT_EQ(events.size(), 1u);
    EXPECT_EQ(events[0]["event"], "BOOKING_CREATED");
    EXPECT_EQ(events[0]["bookingId"], booking["id"]);
    EXPECT_EQ(events[0]["pnr"], booking["pnr"]);
    EXPECT_FALSE(events[0]["createdAt"].get<std::string>().empty());

    auto forbidden = s.client().Get("/api/admin/audit", test::bearer(passenger));
    ASSERT_TRUE(forbidden);
    EXPECT_EQ(forbidden->status, 403);

    auto anonymous = s.client().Get("/api/admin/audit");
    ASSERT_TRUE(anonymous);
    EXPECT_EQ(anonymous->status, 401);

    auto invalidLimit = s.client().Get("/api/admin/audit?limit=0", test::bearer(admin));
    ASSERT_TRUE(invalidLimit);
    EXPECT_EQ(invalidLimit->status, 400);
}
