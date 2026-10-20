#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "TestServer.h"
#include "TrainFixtures.h"
#include "db/Schema.h"
#include "services/TrainService.h"

using namespace rtrs;
using nlohmann::json;

namespace {

struct FaresTest : ::testing::Test {
    Database db{":memory:"};
    TrainService svc{db};
    long long trainId = 0;

    FaresTest() {
        initSchema(db);
        trainId = test::addTrain(db, "90001", "Test Express",
                                 {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                                 {{"2A", "AC 2 Tier", 900, 20},
                                  {"SL", "Sleeper", 250, 100},
                                  {"3A", "AC 3 Tier", 700, 40}});
    }
};

}  // namespace

TEST_F(FaresTest, ListsEveryClassCheapestFirst) {
    auto classes = svc.fares(trainId);

    ASSERT_EQ(classes.size(), 3u);
    EXPECT_EQ(classes[0].code, "SL");
    EXPECT_EQ(classes[0].fare, 250);
    EXPECT_EQ(classes[1].code, "3A");
    EXPECT_EQ(classes[2].code, "2A");
    EXPECT_EQ(classes[2].name, "AC 2 Tier");
}

TEST_F(FaresTest, UnknownTrainHasNoFares) {
    EXPECT_TRUE(svc.fares(9999).empty());
}

TEST(FaresApi, ReturnsClassFares) {
    TestServer s;
    long long id = test::addTrain(s.db(), "90004", "Api Express",
                                  {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                                  {{"SL", "Sleeper", 250, 100}, {"CC", "AC Chair Car", 480, 60}});

    auto res = s.client().Get("/api/trains/" + std::to_string(id) + "/fares");

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    ASSERT_EQ(body["classes"].size(), 2u);
    EXPECT_EQ(body["classes"][0]["code"], "SL");
    EXPECT_EQ(body["classes"][0]["fare"], 250);
    EXPECT_EQ(body["classes"][1]["name"], "AC Chair Car");
    EXPECT_EQ(body["classes"][1]["fare"], 480);
}

TEST(FaresApi, UnknownTrainReturns404) {
    TestServer s;

    auto res = s.client().Get("/api/trains/9999/fares");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
}

TEST(FaresApi, SeededTrainHasFares) {
    TestServer s;
    auto rows = s.db().query("SELECT id FROM trains WHERE number = '12027'");
    ASSERT_EQ(rows.size(), 1u);

    auto res = s.client().Get("/api/trains/" + rows[0][0] + "/fares");

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    ASSERT_EQ(body["classes"].size(), 2u);
    EXPECT_EQ(body["classes"][0]["code"], "CC");
}
