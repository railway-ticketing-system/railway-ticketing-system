#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "TestServer.h"
#include "TrainFixtures.h"
#include "db/Schema.h"
#include "services/TrainService.h"

using namespace rtrs;
using nlohmann::json;

namespace {

struct TrainScheduleTest : ::testing::Test {
    Database db{":memory:"};
    TrainService svc{db};

    TrainScheduleTest() { initSchema(db); }
};

}  // namespace

TEST_F(TrainScheduleTest, ReturnsStopsInSequenceWithEmptyTerminalTimes) {
    long long id = test::addTrain(db, "90001", "Test Express",
                                  {{"Alpha Jn", "", "06:00", 0},
                                   {"Gamma Jn", "07:15", "07:20", 0},
                                   {"Beta Jn", "09:30", "", 1}},
                                  {{"SL", "Sleeper", 250, 100}});

    auto schedule = svc.findById(id);

    ASSERT_TRUE(schedule.has_value());
    EXPECT_EQ(schedule->number, "90001");
    ASSERT_EQ(schedule->stops.size(), 3u);
    EXPECT_EQ(schedule->stops[0].station, "Alpha Jn");
    EXPECT_EQ(schedule->stops[0].arrival, "");
    EXPECT_EQ(schedule->stops[1].station, "Gamma Jn");
    EXPECT_EQ(schedule->stops[1].arrival, "07:15");
    EXPECT_EQ(schedule->stops[2].station, "Beta Jn");
    EXPECT_EQ(schedule->stops[2].dayOffset, 1);
}

TEST_F(TrainScheduleTest, UnknownTrainReturnsNullopt) {
    EXPECT_FALSE(svc.findById(9999).has_value());
}

TEST(ScheduleApi, ReturnsTrainAndStops) {
    TestServer s;
    long long id = test::addTrain(s.db(), "90002", "Api Express",
                                  {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                                  {{"SL", "Sleeper", 250, 100}});

    auto res = s.client().Get("/api/trains/" + std::to_string(id) + "/schedule");

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    EXPECT_EQ(body["train"]["number"], "90002");
    ASSERT_EQ(body["stops"].size(), 2u);
    EXPECT_EQ(body["stops"][0]["station"], "Alpha Jn");
    EXPECT_EQ(body["stops"][1]["station"], "Beta Jn");
    EXPECT_EQ(body["stops"][1]["arrival"], "09:30");
}

TEST(ScheduleApi, UnknownTrainReturns404) {
    TestServer s;

    auto res = s.client().Get("/api/trains/9999/schedule");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST(ScheduleApi, SeededTrainHasSchedule) {
    TestServer s;
    auto rows = s.db().query("SELECT id FROM trains WHERE number = '11007'");
    ASSERT_EQ(rows.size(), 1u);

    auto res = s.client().Get("/api/trains/" + rows[0][0] + "/schedule");

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    EXPECT_EQ(body["stops"].size(), 3u);
}
