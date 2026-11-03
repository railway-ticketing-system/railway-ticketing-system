#include <gtest/gtest.h>
#include "AdminHelpers.h"

using namespace rtrs;
using nlohmann::json;

namespace {
httplib::Result addTrain(TestServer& s, const std::string& token, const json& body) {
    return s.client().Post("/api/admin/trains", test::bearer(token), body.dump(), "application/json");
}

json createTrain(TestServer& s, const std::string& token, const std::string& number = "94001") {
    auto res = addTrain(s, token, test::managedTrainBody(number));
    if (!res || res->status != 201) return json::object();
    return json::parse(res->body)["train"];
}

httplib::Result updateTrain(TestServer& s, const std::string& token, long long id, const json& body) {
    return s.client().Put("/api/admin/trains/" + std::to_string(id), test::bearer(token), body.dump(), "application/json");
}

json bookTrain(TestServer& s, const std::string& token, long long id) {
    auto res = s.client().Post("/api/bookings", test::bearer(token),
        test::bookingBody(id, "SL", {"Asha Rao", "Ravi Kumar"}, "2099-01-01").dump(), "application/json");
    if (!res || res->status != 201) return json::object();
    return json::parse(res->body)["booking"];
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

TEST(AdminTrainUpdateApi, UpdatesSearchScheduleAndFaresWithoutChangingExistingBookingFare) {
    TestServer s;
    auto token = test::adminLogin(s);
    auto train = createTrain(s, token);
    ASSERT_FALSE(train.empty());
    auto booking = bookTrain(s, token, train["id"]);
    ASSERT_FALSE(booking.empty());
    auto body = test::managedTrainBody();
    body["name"] = "Updated Express";
    body["number"] = "94009";
    body["stops"][0]["departure"] = "21:00";
    body["classes"][0]["fare"] = 350;
    body["classes"][0]["totalSeats"] = 12;
    auto res = updateTrain(s, token, train["id"], body);
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200) << res->body;
    auto updated = json::parse(res->body)["train"];
    EXPECT_EQ(updated["name"], "Updated Express");
    EXPECT_EQ(updated["stops"][0]["departure"], "21:00");
    auto search = s.client().Get("/api/trains/search?source=Cedar%20Jn&destination=Maple%20Jn&date=2099-01-01");
    ASSERT_TRUE(search);
    auto matches = json::parse(search->body)["trains"];
    ASSERT_EQ(matches.size(), 1u);
    EXPECT_EQ(matches[0]["number"], "94009");
    EXPECT_EQ(matches[0]["fromFare"], 350);
    EXPECT_EQ(matches[0]["durationMinutes"], 300);
    auto ticket = s.client().Get("/api/bookings/" + booking["pnr"].get<std::string>(), test::bearer(token));
    ASSERT_TRUE(ticket);
    EXPECT_EQ(json::parse(ticket->body)["booking"]["totalFare"], 500);
    TrainService trains(s.db());
    EXPECT_EQ(trains.availability(train["id"], "2099-01-01")[0].availableSeats, 10);
}

TEST(AdminTrainUpdateApi, CannotRemoveReferencedClassOrShrinkBelowAssignedSeats) {
    TestServer s;
    auto token = test::adminLogin(s);
    auto train = createTrain(s, token);
    auto first = bookTrain(s, token, train["id"]);
    auto second = bookTrain(s, token, train["id"]);
    ASSERT_FALSE(first.empty());
    ASSERT_FALSE(second.empty());
    auto cancelled = s.client().Post("/api/bookings/" + std::to_string(first["id"].get<long long>()) +
                                     "/cancel", test::bearer(token), "", "application/json");
    ASSERT_TRUE(cancelled);
    ASSERT_EQ(cancelled->status, 200);
    auto body = test::managedTrainBody();
    body["classes"][0]["totalSeats"] = 2;
    auto shrink = updateTrain(s, token, train["id"], body);
    ASSERT_TRUE(shrink);
    EXPECT_EQ(shrink->status, 409);
    EXPECT_TRUE(json::parse(shrink->body)["fields"].contains("classes.0.totalSeats"));
    body["classes"].erase(body["classes"].begin());
    auto remove = updateTrain(s, token, train["id"], body);
    ASSERT_TRUE(remove);
    EXPECT_EQ(remove->status, 409);
    EXPECT_TRUE(json::parse(remove->body)["fields"].contains("classes"));
    auto stillValid = s.client().Get("/api/admin/trains/" + std::to_string(train["id"].get<long long>()), test::bearer(token));
    ASSERT_TRUE(stillValid);
    EXPECT_EQ(json::parse(stillValid->body)["train"], train);
}

TEST(AdminTrainUpdateApi, CanAddClassesAndRemoveUnusedClasses) {
    TestServer s;
    auto token = test::adminLogin(s);
    auto train = createTrain(s, token);
    auto body = test::managedTrainBody();
    body["classes"][1] = {{"code", "CC"}, {"name", "Chair Car"}, {"fare", 450}, {"totalSeats", 20}};
    auto res = updateTrain(s, token, train["id"], body);
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto classes = json::parse(res->body)["train"]["classes"];
    ASSERT_EQ(classes.size(), 2u);
    EXPECT_EQ(classes[1]["code"], "CC");
}

TEST(AdminTrainUpdateApi, RejectsUnauthorizedInvalidUnknownAndDuplicateUpdates) {
    TestServer s;
    auto token = test::adminLogin(s);
    auto train = createTrain(s, token);
    createTrain(s, token, "94002");
    auto passenger = test::registerAndLogin(s);
    auto body = test::managedTrainBody();
    auto denied = updateTrain(s, passenger, train["id"], body);
    ASSERT_TRUE(denied);
    EXPECT_EQ(denied->status, 403);
    auto missing = updateTrain(s, "", train["id"], body);
    ASSERT_TRUE(missing);
    EXPECT_EQ(missing->status, 401);
    auto unknown = updateTrain(s, token, 99999, body);
    ASSERT_TRUE(unknown);
    EXPECT_EQ(unknown->status, 404);
    body["number"] = "94002";
    auto duplicate = updateTrain(s, token, train["id"], body);
    ASSERT_TRUE(duplicate);
    EXPECT_EQ(duplicate->status, 409);
    body["stops"][0]["departure"] = "99:00";
    auto invalid = updateTrain(s, token, train["id"], body);
    ASSERT_TRUE(invalid);
    EXPECT_EQ(invalid->status, 400);
    EXPECT_TRUE(json::parse(invalid->body)["fields"].contains("stops.0.departure"));
}

TEST(AdminTrainUpdateApi, FailedClassUpdateRollsBackTrainAndStopChanges) {
    TestServer s;
    auto token = test::adminLogin(s);
    auto train = createTrain(s, token);
    s.db().execute("CREATE TRIGGER reject_class_update BEFORE UPDATE ON train_classes"
                   " BEGIN SELECT RAISE(ABORT, 'write unavailable'); END");
    auto body = test::managedTrainBody();
    body["name"] = "Changed Name";
    body["stops"][0]["departure"] = "21:00";
    auto res = updateTrain(s, token, train["id"], body);
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 500);
    auto current = s.client().Get("/api/admin/trains/" + std::to_string(train["id"].get<long long>()), test::bearer(token));
    ASSERT_TRUE(current);
    EXPECT_EQ(json::parse(current->body)["train"], train);
}
