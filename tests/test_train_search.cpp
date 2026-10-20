#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "TestServer.h"
#include "db/Schema.h"
#include "services/TrainService.h"

using namespace rtrs;
using nlohmann::json;

namespace {

struct StopIn {
    std::string station, arrival, departure;
    int day = 0;
};

struct ClassIn {
    std::string code, name;
    int fare = 0;
    int seats = 100;
};

long long addTrain(Database& db, const std::string& number, const std::string& name,
                   const std::vector<StopIn>& stops, const std::vector<ClassIn>& classes) {
    db.execute("INSERT INTO trains (number, name) VALUES (?, ?)", {number, name});
    long long id = db.lastInsertId();
    int seq = 1;
    for (const auto& stop : stops) {
        db.execute(
            "INSERT INTO train_stops (train_id, station, arrival_time, departure_time, day_offset, seq)"
            " VALUES (?, ?, ?, ?, ?, ?)",
            {std::to_string(id), stop.station, stop.arrival, stop.departure, std::to_string(stop.day),
             std::to_string(seq++)});
    }
    for (const auto& cls : classes) {
        db.execute(
            "INSERT INTO train_classes (train_id, class_code, class_name, fare, total_seats)"
            " VALUES (?, ?, ?, ?, ?)",
            {std::to_string(id), cls.code, cls.name, std::to_string(cls.fare), std::to_string(cls.seats)});
    }
    return id;
}

const std::vector<StopIn> kAlphaToBeta = {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}};
const std::vector<ClassIn> kClasses = {{"SL", "Sleeper", 250, 100}, {"3A", "AC 3 Tier", 700, 40}};

struct TrainSearchTest : ::testing::Test {
    Database db{":memory:"};
    TrainService svc{db};

    TrainSearchTest() { initSchema(db); }
};

SearchQuery query(const std::string& source, const std::string& destination, const std::string& date) {
    SearchQuery q;
    q.source = source;
    q.destination = destination;
    q.date = date;
    return q;
}

}  // namespace

TEST_F(TrainSearchTest, FindsDirectTrainAndComputesDuration) {
    addTrain(db, "90001", "Test Express", kAlphaToBeta, kClasses);

    auto result = svc.search(query("Alpha Jn", "Beta Jn", "2026-11-01"));

    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.trains.size(), 1u);
    EXPECT_EQ(result.trains[0].name, "Test Express");
    EXPECT_EQ(result.trains[0].departure, "06:00");
    EXPECT_EQ(result.trains[0].arrival, "09:30");
    EXPECT_EQ(result.trains[0].durationMinutes, 210);
    EXPECT_EQ(result.trains[0].fromFare, 250);
}

TEST_F(TrainSearchTest, MatchesStationsCaseInsensitively) {
    addTrain(db, "90001", "Test Express", kAlphaToBeta, kClasses);

    auto result = svc.search(query("alpha jn", "BETA JN", "2026-11-01"));

    ASSERT_TRUE(result.ok());
    EXPECT_EQ(result.trains.size(), 1u);
}

TEST_F(TrainSearchTest, UsesIntermediateStops) {
    addTrain(db, "90001", "Test Express",
             {{"Alpha Jn", "", "06:00", 0},
              {"Gamma Jn", "07:15", "07:20", 0},
              {"Beta Jn", "09:30", "", 0}},
             kClasses);

    auto result = svc.search(query("Gamma Jn", "Beta Jn", "2026-11-01"));

    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.trains.size(), 1u);
    EXPECT_EQ(result.trains[0].durationMinutes, 130);
}

TEST_F(TrainSearchTest, HandlesOvernightJourney) {
    addTrain(db, "90001", "Night Express",
             {{"Alpha Jn", "", "22:00", 0}, {"Beta Jn", "02:30", "", 1}}, kClasses);

    auto result = svc.search(query("Alpha Jn", "Beta Jn", "2026-11-01"));

    ASSERT_TRUE(result.ok());
    ASSERT_EQ(result.trains.size(), 1u);
    EXPECT_EQ(result.trains[0].durationMinutes, 270);
}

TEST_F(TrainSearchTest, ReverseDirectionReturnsNothing) {
    addTrain(db, "90001", "Test Express", kAlphaToBeta, kClasses);

    auto result = svc.search(query("Beta Jn", "Alpha Jn", "2026-11-01"));

    ASSERT_TRUE(result.ok());
    EXPECT_TRUE(result.trains.empty());
}

TEST_F(TrainSearchTest, RequiresSourceDestinationAndDate) {
    auto result = svc.search(query("", "", ""));

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.errors.count("source"));
    EXPECT_TRUE(result.errors.count("destination"));
    EXPECT_TRUE(result.errors.count("date"));
}

TEST_F(TrainSearchTest, RejectsSameSourceAndDestination) {
    auto result = svc.search(query("Pune Jn", "pune jn", "2026-11-01"));

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.errors.count("destination"));
}

TEST_F(TrainSearchTest, RejectsImpossibleCalendarDate) {
    auto result = svc.search(query("Alpha Jn", "Beta Jn", "2026-02-30"));

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.errors.count("date"));
}

TEST_F(TrainSearchTest, SqlInjectionInStationIsRejected) {
    addTrain(db, "90001", "Test Express", kAlphaToBeta, kClasses);

    auto result = svc.search(query("Alpha' OR 1=1--", "Beta Jn", "2026-11-01"));

    EXPECT_FALSE(result.ok());
    EXPECT_TRUE(result.errors.count("source"));
    EXPECT_EQ(db.query("SELECT COUNT(*) FROM trains")[0][0], "1");
}

TEST(SearchApi, ReturnsMatchingTrainsAsJson) {
    TestServer s;
    addTrain(s.db(), "90002", "Api Express", kAlphaToBeta, kClasses);

    httplib::Params params{{"source", "Alpha Jn"}, {"destination", "Beta Jn"}, {"date", "2026-11-01"}};
    auto res = s.client().Get("/api/trains/search", params, httplib::Headers{});

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    EXPECT_EQ(body["count"], 1);
    EXPECT_EQ(body["trains"][0]["number"], "90002");
    EXPECT_EQ(body["trains"][0]["durationMinutes"], 210);
    EXPECT_EQ(body["date"], "2026-11-01");
}

TEST(SearchApi, MissingParametersReturnFieldErrors) {
    TestServer s;

    auto res = s.client().Get("/api/trains/search", httplib::Params{}, httplib::Headers{});

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    auto body = json::parse(res->body);
    EXPECT_TRUE(body["fields"].count("source"));
    EXPECT_TRUE(body["fields"].count("destination"));
    EXPECT_TRUE(body["fields"].count("date"));
}

TEST(SearchApi, NoMatchReturnsEmptyList) {
    TestServer s;

    httplib::Params params{{"source", "Nowhere Jn"}, {"destination", "Elsewhere Jn"}, {"date", "2026-11-01"}};
    auto res = s.client().Get("/api/trains/search", params, httplib::Headers{});

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    EXPECT_EQ(body["count"], 0);
    EXPECT_TRUE(body["trains"].empty());
}

TEST(SearchApi, SeededRouteIsSearchable) {
    TestServer s;

    httplib::Params params{{"source", "Pune Jn"}, {"destination", "Mumbai CSMT"}, {"date", "2026-11-01"}};
    auto res = s.client().Get("/api/trains/search", params, httplib::Headers{});

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    EXPECT_EQ(body["count"], 2);
}
