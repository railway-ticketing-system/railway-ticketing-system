#include <gtest/gtest.h>
#include "AdminHelpers.h"

using namespace rtrs;
using nlohmann::json;

namespace {
httplib::Result addTrain(TestServer& s, const std::string& token, const json& body) {
    return s.client().Post("/api/admin/trains", test::bearer(token), body.dump(), "application/json");
}
}

TEST(AdminTrainCreateApi, CreatesCompleteTrainVisibleInPassengerSearchAndDetails) {
    TestServer s;
    auto token = test::adminLogin(s);
    auto res = addTrain(s, token, test::managedTrainBody());
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 201) << res->body;
    auto train = json::parse(res->body)["train"];
    EXPECT_EQ(train["stops"].size(), 3u);
    EXPECT_EQ(train["classes"].size(), 2u);
    auto id = std::to_string(train["id"].get<long long>());
    auto schedule = s.client().Get("/api/trains/" + id + "/schedule");
    ASSERT_TRUE(schedule);
    EXPECT_EQ(schedule->status, 200);
    auto search = s.client().Get("/api/trains/search?source=Cedar%20Jn&destination=Maple%20Jn&date=2026-11-15");
    ASSERT_TRUE(search);
    ASSERT_EQ(search->status, 200);
    auto matches = json::parse(search->body)["trains"];
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0]["number"], "94001");
    EXPECT_EQ(matches[0]["durationMinutes"], 240);
    auto detail = s.client().Get("/api/admin/trains/" + id, test::bearer(token));
    ASSERT_TRUE(detail);
    EXPECT_EQ(json::parse(detail->body)["train"], train);
}

TEST(AdminTrainCreateApi, ReadAndWriteRequireAnActiveAdminSession) {
    TestServer s;
    auto passenger = test::registerAndLogin(s);
    for (const auto& path : {"/api/admin/trains", "/api/admin/trains/1"}) {
        auto missing = s.client().Get(path);
        ASSERT_TRUE(missing);
        EXPECT_EQ(missing->status, 401);
        auto denied = s.client().Get(path, test::bearer(passenger));
        ASSERT_TRUE(denied);
        EXPECT_EQ(denied->status, 403);
    }
    auto denied = addTrain(s, passenger, test::managedTrainBody());
    ASSERT_TRUE(denied);
    EXPECT_EQ(denied->status, 403);
    auto missing = s.client().Post("/api/admin/trains", test::managedTrainBody().dump(), "application/json");
    ASSERT_TRUE(missing);
    EXPECT_EQ(missing->status, 401);
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM trains WHERE number = '94001'")[0][0], "0");
}

TEST(AdminTrainCreateApi, InvalidScheduleAndClassValuesReturnIndexedErrorsAndWriteNothing) {
    TestServer s;
    auto token = test::adminLogin(s);
    auto body = test::managedTrainBody();
    body["number"] = "oops";
    body["stops"][2]["dayOffset"] = 0;
    body["stops"][1]["departure"] = "22:30";
    body["classes"][0]["fare"] = 0;
    body["classes"][1]["code"] = "SL";
    body["classes"][1]["totalSeats"] = "4";
    auto res = addTrain(s, token, body);
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 400);
    auto fields = json::parse(res->body)["fields"];
    for (const auto& key : {"number", "stops.2.arrival", "stops.1.departure", "classes.0.fare", "classes.1.code", "classes.1.totalSeats"})
        EXPECT_TRUE(fields.contains(key)) << key;
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM trains WHERE name = 'Admin Night Express'")[0][0], "0");
}

TEST(AdminTrainCreateApi, RejectsMalformedTypesMissingCollectionsAndDuplicateStations) {
    TestServer s;
    auto token = test::adminLogin(s);
    for (const auto& raw : {"{oops", "[]", "null"}) {
        auto res = s.client().Post("/api/admin/trains", test::bearer(token), raw, "application/json");
        ASSERT_TRUE(res);
        EXPECT_EQ(res->status, 400);
    }
    auto body = test::managedTrainBody();
    body["stops"][1]["station"] = " cedar jn ";
    body["stops"][2]["dayOffset"] = 18446744073709551615ull;
    body["classes"] = json::array();
    auto res = addTrain(s, token, body);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    auto fields = json::parse(res->body)["fields"];
    EXPECT_TRUE(fields.contains("stops.1.station"));
    EXPECT_TRUE(fields.contains("stops.2.dayOffset"));
    EXPECT_TRUE(fields.contains("classes"));
}

TEST(AdminTrainCreateApi, DuplicateTrainNumberReturnsConflict) {
    TestServer s;
    auto token = test::adminLogin(s);
    auto first = addTrain(s, token, test::managedTrainBody());
    ASSERT_TRUE(first);
    ASSERT_EQ(first->status, 201);
    auto duplicate = addTrain(s, token, test::managedTrainBody());
    ASSERT_TRUE(duplicate);
    EXPECT_EQ(duplicate->status, 409);
    EXPECT_TRUE(json::parse(duplicate->body)["fields"].contains("number"));
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM trains WHERE number = '94001'")[0][0], "1");
}

TEST(AdminTrainCreateApi, DatabaseFailureRollsBackTrainStopsAndClasses) {
    TestServer s;
    auto token = test::adminLogin(s);
    s.db().execute("CREATE TRIGGER reject_new_class BEFORE INSERT ON train_classes"
                   " BEGIN SELECT RAISE(ABORT, 'write unavailable'); END");
    auto res = addTrain(s, token, test::managedTrainBody());
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 500);
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM trains WHERE number = '94001'")[0][0], "0");
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM train_stops WHERE station = 'Cedar Jn'")[0][0], "0");
}
