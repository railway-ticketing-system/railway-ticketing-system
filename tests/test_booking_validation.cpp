#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;
using nlohmann::json;

namespace {

long long addBookingTrain(TestServer& s) {
    return test::addTrain(s.db(), "93002", "Validation Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, 10}});
}

json previewFor(TestServer& s, const std::string& token, const json& body) {
    auto res = s.client().Post("/api/bookings/preview", test::bearer(token), body.dump(),
                               "application/json");
    if (!res) return json::object();
    return json::parse(res->body);
}

}  // namespace

TEST(BookingValidationApi, RejectsInvalidNameAgeAndGender) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId);
    body["passengers"] = json::array({test::passenger("A", 0, "X")});

    auto response = previewFor(s, token, body);

    ASSERT_TRUE(response.contains("fields"));
    EXPECT_EQ(response["fields"]["passengers.0.name"], "Enter the passenger's name.");
    EXPECT_EQ(response["fields"]["passengers.0.age"], "Age must be between 1 and 120.");
    EXPECT_EQ(response["fields"]["passengers.0.gender"], "Select the passenger's gender.");
}

TEST(BookingValidationApi, RejectsNonNumericAndOutOfRangeAge) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId);
    body["passengers"] = json::array({test::passenger("Asha Rao", 121)});

    auto tooOld = previewFor(s, token, body);
    body["passengers"][0]["age"] = "twenty";
    auto notNumeric = previewFor(s, token, body);

    EXPECT_EQ(tooOld["fields"]["passengers.0.age"], "Age must be between 1 and 120.");
    EXPECT_EQ(notNumeric["fields"]["passengers.0.age"], "Age must be between 1 and 120.");
}

TEST(BookingValidationApi, ReportsTheCorrectPassengerIndex) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar"});
    body["passengers"][1]["name"] = "R2";

    auto response = previewFor(s, token, body);

    EXPECT_TRUE(response["fields"].count("passengers.1.name"));
    EXPECT_FALSE(response["fields"].count("passengers.0.name"));
}

TEST(BookingValidationApi, RejectsNamesWithDigits) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId, "SL", {"Passenger 1"});

    auto response = previewFor(s, token, body);

    EXPECT_TRUE(response["fields"].count("passengers.0.name"));
}

TEST(BookingValidationApi, AcceptsValidPassengersWithAgeAsText) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar"});
    body["passengers"][0]["name"] = "  Asha Rao  ";
    body["passengers"][0]["age"] = "1";
    body["passengers"][1]["age"] = 120;
    body["passengers"][1]["gender"] = "F";

    auto response = previewFor(s, token, body);

    EXPECT_FALSE(response.contains("fields"));
    EXPECT_EQ(response["preview"]["passengerCount"], 2);
}
