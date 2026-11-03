#include <gtest/gtest.h>

#include <ctime>
#include <string>

#include <nlohmann/json.hpp>

#include "AdminHelpers.h"
#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"
#include "util/Validation.h"

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

long long addInputTrain(TestServer& s) {
    return test::addTrain(s.db(), "98001", "Input Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, 10}});
}

}  // namespace

TEST(ValidationInput, ControlCharactersAreRejected) {
    EXPECT_TRUE(hasControlChars(std::string("secret\n123")));
    EXPECT_TRUE(hasControlChars(std::string("bad\0name", 8)));
    EXPECT_FALSE(hasControlChars("Asha Rao"));
    EXPECT_FALSE(isValidPassword("secret\n123"));
    EXPECT_TRUE(isValidPassword("secret123"));
}

// RTS-SR-004 / TC-SEC-05: injected or script-like input is validated safely.
TEST(InputValidationApi, ScriptAndInjectionInputsAreRejected) {
    TestServer s;
    auto scriptName = s.client().Post(
        "/api/auth/register",
        json{{"name", "<script>alert(1)</script>"}, {"email", "script@example.com"},
             {"phone", "9876543210"}, {"password", "secret123"}}.dump(),
        "application/json");
    ASSERT_TRUE(scriptName);
    EXPECT_EQ(scriptName->status, 400);
    EXPECT_TRUE(json::parse(scriptName->body).contains("fields"));

    auto injection = s.client().Post(
        "/api/auth/register",
        json{{"name", "Asha Rao"}, {"email", "a' OR '1'='1@example.com"},
             {"phone", "9876543210"}, {"password", "secret123"}}.dump(),
        "application/json");
    ASSERT_TRUE(injection);
    EXPECT_EQ(injection->status, 400);

    auto control = s.client().Post(
        "/api/auth/register",
        json{{"name", "Asha Rao"}, {"email", "control@example.com"},
             {"phone", "9876543210"}, {"password", "secret123\n"}}.dump(),
        "application/json");
    ASSERT_TRUE(control);
    EXPECT_EQ(control->status, 400);

    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM users")[0][0], "0");
}

TEST(InputValidationApi, OverlongAndScriptPassengerNamesAreRejected) {
    TestServer s;
    long long trainId = addInputTrain(s);
    std::string token = test::registerAndLogin(s, "booker@example.com");
    auto script = s.client().Post("/api/bookings", test::bearer(token),
                                  test::bookingBody(trainId, "SL", {"<script>x</script>"}, dateAfter(3)).dump(),
                                  "application/json");
    ASSERT_TRUE(script);
    EXPECT_EQ(script->status, 400);

    auto overlong = s.client().Post("/api/bookings", test::bearer(token),
                                    test::bookingBody(trainId, "SL", {std::string(200, 'A')}, dateAfter(3)).dump(),
                                    "application/json");
    ASSERT_TRUE(overlong);
    EXPECT_EQ(overlong->status, 400);
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM bookings")[0][0], "0");
}

TEST(InputValidationApi, OverlongSearchParametersAreRejected) {
    TestServer s;
    std::string source(120, 'A');
    auto res = s.client().Get(("/api/trains/search?source=" + source + "&destination=Beta&date=2026-11-01").c_str());
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_EQ(json::parse(res->body)["error"], "Please correct the highlighted fields.");
}

TEST(InputValidationApi, AdminTrainNamesAndFaresAreValidated) {
    TestServer s;
    std::string admin = test::adminLogin(s);
    const std::string trainsBefore = s.db().query("SELECT COUNT(*) FROM trains")[0][0];
    auto body = test::managedTrainBody("98002");
    body["name"] = "<script>alert(1)</script>";
    auto script = s.client().Post("/api/admin/trains", test::bearer(admin), body.dump(), "application/json");
    ASSERT_TRUE(script);
    EXPECT_EQ(script->status, 400);

    auto negative = test::managedTrainBody("98003");
    negative["classes"][0]["fare"] = -5;
    auto badFare = s.client().Post("/api/admin/trains", test::bearer(admin), negative.dump(), "application/json");
    ASSERT_TRUE(badFare);
    EXPECT_EQ(badFare->status, 400);
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM trains")[0][0], trainsBefore);
}

TEST(InputValidationApi, OversizedPathIdentifiersReturnNotFound) {
    TestServer s;
    std::string token = test::registerAndLogin(s, "path@example.com");
    std::string pnr(100, '9');
    auto res = s.client().Get(("/api/bookings/" + pnr).c_str(), test::bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
    EXPECT_EQ(s.client().Get("/api/health")->status, 200);
}

TEST(InputValidationApi, WrongJsonTypesCannotBypassValidation) {
    TestServer s;
    std::string token = test::registerAndLogin(s, "types@example.com");
    auto res = s.client().Put("/api/users/me", test::bearer(token),
                              json{{"name", 123}, {"phone", true}}.dump(), "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    auto profile = s.client().Get("/api/users/me", test::bearer(token));
    ASSERT_TRUE(profile);
    EXPECT_EQ(json::parse(profile->body)["user"]["name"], "Asha Rao");
}
