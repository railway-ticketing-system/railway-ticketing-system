#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "TestServer.h"
#include "TrainFixtures.h"
#include "db/Schema.h"
#include "services/TrainService.h"

using namespace rtrs;
using nlohmann::json;

namespace {

struct AvailabilityTest : ::testing::Test {
    Database db{":memory:"};
    TrainService svc{db};
    long long trainId = 0;

    AvailabilityTest() {
        initSchema(db);
        trainId = test::addTrain(db, "90001", "Test Express",
                                 {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                                 {{"SL", "Sleeper", 250, 100}, {"3A", "AC 3 Tier", 700, 40}});
    }
};

}  // namespace

TEST_F(AvailabilityTest, NewTrainHasEverySeatAvailable) {
    auto entries = svc.availability(trainId, "2026-11-01");

    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[0].code, "SL");
    EXPECT_EQ(entries[0].totalSeats, 100);
    EXPECT_EQ(entries[0].bookedSeats, 0);
    EXPECT_EQ(entries[0].availableSeats, 100);
    EXPECT_EQ(entries[0].fare, 250);
    EXPECT_EQ(entries[1].availableSeats, 40);
}

TEST_F(AvailabilityTest, ReservationsReduceAvailabilityForThatDateOnly) {
    test::reserveSeats(db, trainId, "2026-11-01", "SL", 30);
    test::reserveSeats(db, trainId, "2026-11-01", "SL", 15);
    test::reserveSeats(db, trainId, "2026-11-02", "SL", 5);

    auto first = svc.availability(trainId, "2026-11-01");
    auto second = svc.availability(trainId, "2026-11-02");

    EXPECT_EQ(first[0].bookedSeats, 45);
    EXPECT_EQ(first[0].availableSeats, 55);
    EXPECT_EQ(second[0].bookedSeats, 5);
    EXPECT_EQ(second[0].availableSeats, 95);
}

TEST_F(AvailabilityTest, SoldOutClassNeverReportsNegativeSeats) {
    test::reserveSeats(db, trainId, "2026-11-01", "3A", 60);

    auto entries = svc.availability(trainId, "2026-11-01");

    ASSERT_EQ(entries.size(), 2u);
    EXPECT_EQ(entries[1].availableSeats, 0);
}

TEST_F(AvailabilityTest, UnknownTrainHasNoClasses) {
    EXPECT_TRUE(svc.availability(9999, "2026-11-01").empty());
}

TEST(AvailabilityApi, ReturnsClassWiseAvailability) {
    TestServer s;
    long long id = test::addTrain(s.db(), "90003", "Api Express",
                                  {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                                  {{"SL", "Sleeper", 250, 100}, {"2A", "AC 2 Tier", 900, 20}});
    test::reserveSeats(s.db(), id, "2026-11-01", "SL", 25);

    auto res = s.client().Get("/api/trains/" + std::to_string(id) + "/availability?date=2026-11-01");

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    EXPECT_EQ(body["date"], "2026-11-01");
    ASSERT_EQ(body["classes"].size(), 2u);
    EXPECT_EQ(body["classes"][0]["code"], "SL");
    EXPECT_EQ(body["classes"][0]["availableSeats"], 75);
    EXPECT_EQ(body["classes"][1]["code"], "2A");
}

TEST(AvailabilityApi, MissingOrInvalidDateReturns400) {
    TestServer s;
    auto rows = s.db().query("SELECT id FROM trains WHERE number = '11007'");
    ASSERT_EQ(rows.size(), 1u);

    auto missing = s.client().Get("/api/trains/" + rows[0][0] + "/availability");
    auto invalid = s.client().Get("/api/trains/" + rows[0][0] + "/availability?date=2026-02-30");

    ASSERT_TRUE(missing);
    ASSERT_TRUE(invalid);
    EXPECT_EQ(missing->status, 400);
    EXPECT_EQ(invalid->status, 400);
    EXPECT_EQ(json::parse(invalid->body)["fields"]["date"], "Enter a valid travel date (YYYY-MM-DD).");
}

TEST(AvailabilityApi, UnknownTrainReturns404) {
    TestServer s;

    auto res = s.client().Get("/api/trains/9999/availability?date=2026-11-01");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}
