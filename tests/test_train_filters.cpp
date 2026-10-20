#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "TestServer.h"
#include "TrainFixtures.h"
#include "db/Schema.h"
#include "services/TrainService.h"

using namespace rtrs;
using nlohmann::json;

namespace {

struct TrainFilterTest : ::testing::Test {
    Database db{":memory:"};
    TrainService svc{db};

    TrainFilterTest() {
        initSchema(db);
        test::addTrain(db, "91001", "Cheap Express",
                       {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:00", "", 0}},
                       {{"SL", "Sleeper", 200, 100}, {"3A", "AC 3 Tier", 600, 40}});
        test::addTrain(db, "91002", "Chair Express",
                       {{"Alpha Jn", "", "12:00", 0}, {"Beta Jn", "18:00", "", 0}},
                       {{"CC", "AC Chair Car", 500, 80}});
        test::addTrain(db, "91003", "Evening Express",
                       {{"Alpha Jn", "", "18:00", 0}, {"Beta Jn", "21:00", "", 0}},
                       {{"SL", "Sleeper", 400, 90}});
        test::addTrain(db, "91004", "Premium Express",
                       {{"Alpha Jn", "", "20:00", 0}, {"Beta Jn", "23:00", "", 0}},
                       {{"1A", "AC First Class", 5000, 20}});
    }
};

SearchQuery baseQuery() {
    SearchQuery q;
    q.source = "Alpha Jn";
    q.destination = "Beta Jn";
    q.date = "2026-11-01";
    return q;
}

std::vector<std::string> numbers(const SearchResult& result) {
    std::vector<std::string> out;
    for (const auto& train : result.trains) out.push_back(train.number);
    return out;
}

using Numbers = std::vector<std::string>;

}  // namespace

TEST_F(TrainFilterTest, FiltersByTravelClass) {
    auto q = baseQuery();
    q.travelClass = "3A";

    auto result = svc.search(q);

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(numbers(result), Numbers{"91001"});
    EXPECT_EQ(result.trains[0].fromFare, 600);
}

TEST_F(TrainFilterTest, FiltersByMaximumFare) {
    auto q = baseQuery();
    q.maxFare = "450";

    auto result = svc.search(q);

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(numbers(result), (Numbers{"91001", "91003"}));
}

TEST_F(TrainFilterTest, MaximumFareAppliesToChosenClass) {
    auto q = baseQuery();
    q.travelClass = "CC";
    q.maxFare = "550";

    auto result = svc.search(q);

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(numbers(result), Numbers{"91002"});
}

TEST_F(TrainFilterTest, FiltersByDepartureWindow) {
    auto q = baseQuery();
    q.departureFrom = "10:00";
    q.departureTo = "19:00";

    auto result = svc.search(q);

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(numbers(result), (Numbers{"91002", "91003"}));
}

TEST_F(TrainFilterTest, SortsByPriceAscendingAndDescending) {
    auto ascending = baseQuery();
    ascending.sort = "price";
    auto descending = ascending;
    descending.order = "desc";

    EXPECT_EQ(numbers(svc.search(ascending)), (Numbers{"91001", "91003", "91002", "91004"}));
    EXPECT_EQ(numbers(svc.search(descending)), (Numbers{"91004", "91002", "91003", "91001"}));
}

TEST_F(TrainFilterTest, SortsByDurationWithDepartureTieBreak) {
    auto q = baseQuery();
    q.sort = "duration";

    EXPECT_EQ(numbers(svc.search(q)), (Numbers{"91001", "91003", "91004", "91002"}));
}

TEST_F(TrainFilterTest, SortsByDepartureDescending) {
    auto q = baseQuery();
    q.sort = "departure";
    q.order = "desc";

    EXPECT_EQ(numbers(svc.search(q)), (Numbers{"91004", "91003", "91002", "91001"}));
}

TEST_F(TrainFilterTest, InvalidFiltersAreRejected) {
    auto q = baseQuery();
    q.travelClass = "XX";
    q.maxFare = "abc";
    q.departureFrom = "25:00";
    q.sort = "name";
    q.order = "up";

    auto result = svc.search(q);

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.errors.count("class"));
    EXPECT_TRUE(result.errors.count("maxFare"));
    EXPECT_TRUE(result.errors.count("departureFrom"));
    EXPECT_TRUE(result.errors.count("sort"));
    EXPECT_TRUE(result.errors.count("order"));
}

TEST_F(TrainFilterTest, DepartureWindowMustBeOrdered) {
    auto q = baseQuery();
    q.departureFrom = "18:00";
    q.departureTo = "06:00";

    auto result = svc.search(q);

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.errors.count("departureTo"));
}

TEST(FilterApi, AppliesClassFareAndSortParameters) {
    TestServer s;
    test::addTrain(s.db(), "92001", "Api Cheap",
                   {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:00", "", 0}},
                   {{"SL", "Sleeper", 200, 100}});
    test::addTrain(s.db(), "92002", "Api Evening",
                   {{"Alpha Jn", "", "18:00", 0}, {"Beta Jn", "21:00", "", 0}},
                   {{"SL", "Sleeper", 400, 100}});
    test::addTrain(s.db(), "92003", "Api Chair",
                   {{"Alpha Jn", "", "12:00", 0}, {"Beta Jn", "15:00", "", 0}},
                   {{"CC", "AC Chair Car", 350, 100}});

    httplib::Params params{{"source", "Alpha Jn"},
                           {"destination", "Beta Jn"},
                           {"date", "2026-11-01"},
                           {"class", "SL"},
                           {"maxFare", "500"},
                           {"sort", "price"},
                           {"order", "desc"}};
    auto res = s.client().Get("/api/trains/search", params, httplib::Headers{});

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    ASSERT_EQ(body["count"], 2);
    EXPECT_EQ(body["trains"][0]["number"], "92002");
    EXPECT_EQ(body["trains"][1]["number"], "92001");
}

TEST(FilterApi, InvalidFilterReturnsFieldError) {
    TestServer s;

    httplib::Params params{{"source", "Alpha Jn"},
                           {"destination", "Beta Jn"},
                           {"date", "2026-11-01"},
                           {"class", "XX"}};
    auto res = s.client().Get("/api/trains/search", params, httplib::Headers{});

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_EQ(json::parse(res->body)["fields"]["class"], "Choose a valid travel class.");
}
