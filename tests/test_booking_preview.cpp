#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;
using nlohmann::json;

namespace {

long long addBookingTrain(TestServer& s) {
    return test::addTrain(s.db(), "93001", "Booking Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, 10}, {"3A", "AC 3 Tier", 700, 4}});
}

httplib::Result postPreview(TestServer& s, const std::string& token, const json& body) {
    return s.client().Post("/api/bookings/preview", test::bearer(token), body.dump(), "application/json");
}

}  // namespace

TEST(BookingPreviewApi, RequiresAuthentication) {
    TestServer s;
    long long trainId = addBookingTrain(s);

    auto res = s.client().Post("/api/bookings/preview", test::bookingBody(trainId).dump(),
                               "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 401);
}

TEST(BookingPreviewApi, ReturnsFareAndAvailability) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);

    auto res = postPreview(s, token, test::bookingBody(trainId, "SL", {"Asha Rao", "Ravi Kumar"}));

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto preview = json::parse(res->body)["preview"];
    EXPECT_EQ(preview["trainNumber"], "93001");
    EXPECT_EQ(preview["trainName"], "Booking Express");
    EXPECT_EQ(preview["class"], "SL");
    EXPECT_EQ(preview["className"], "Sleeper");
    EXPECT_EQ(preview["farePerPassenger"], 250);
    EXPECT_EQ(preview["passengerCount"], 2);
    EXPECT_EQ(preview["totalFare"], 500);
    EXPECT_EQ(preview["availableSeats"], 10);
    EXPECT_EQ(preview["date"], "2026-11-01");
}

TEST(BookingPreviewApi, ReflectsExistingReservations) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    test::reserveSeats(s.db(), trainId, "2026-11-01", "SL", 3);

    auto res = postPreview(s, token, test::bookingBody(trainId));

    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    EXPECT_EQ(json::parse(res->body)["preview"]["availableSeats"], 7);
}

TEST(BookingPreviewApi, RejectsMissingPassengers) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId);
    body["passengers"] = json::array();

    auto res = postPreview(s, token, body);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_TRUE(json::parse(res->body)["fields"].count("passengers"));
}

TEST(BookingPreviewApi, RejectsMoreThanSixPassengers) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId, "SL",
                                  {"Asha Rao", "Ravi Kumar", "Meera Nair", "Karan Shah", "Tara Iyer",
                                   "Vikram Singh", "Nina Bose"});

    auto res = postPreview(s, token, body);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_EQ(json::parse(res->body)["fields"]["passengers"], "A booking can have at most 6 passengers.");
}

TEST(BookingPreviewApi, RejectsInvalidDateAndClass) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    auto body = test::bookingBody(trainId, "XX", {"Asha Rao"}, "2026-02-30");

    auto res = postPreview(s, token, body);

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    auto fields = json::parse(res->body)["fields"];
    EXPECT_TRUE(fields.count("date"));
    EXPECT_TRUE(fields.count("class"));
}

TEST(BookingPreviewApi, RejectsUnknownTrainAndClassNotOffered) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);

    auto unknownTrain = postPreview(s, token, test::bookingBody(9999));
    auto missingClass = postPreview(s, token, test::bookingBody(trainId, "1A"));

    ASSERT_TRUE(unknownTrain);
    ASSERT_TRUE(missingClass);
    EXPECT_EQ(unknownTrain->status, 400);
    EXPECT_EQ(json::parse(unknownTrain->body)["fields"]["trainId"], "Train not found.");
    EXPECT_EQ(missingClass->status, 400);
    EXPECT_EQ(json::parse(missingClass->body)["fields"]["class"],
              "This class is not available on the selected train.");
}

TEST(BookingPreviewApi, RejectsMalformedJson) {
    TestServer s;
    std::string token = test::registerAndLogin(s);

    auto res = s.client().Post("/api/bookings/preview", test::bearer(token), "{oops", "application/json");

    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
}
