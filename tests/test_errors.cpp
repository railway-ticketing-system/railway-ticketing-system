#include <gtest/gtest.h>

#include <string>

#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"

using namespace rtrs;
using nlohmann::json;

namespace {

void expectSafeJson(const httplib::Result& res) {
    ASSERT_TRUE(res);
    auto body = json::parse(res->body, nullptr, false);
    ASSERT_FALSE(body.is_discarded());
    EXPECT_TRUE(body.contains("error"));
    std::string text = res->body;
    for (const char* leak : {"sqlite", "SQL", "stack", "exception", "password_hash", "token_hash"})
        EXPECT_EQ(text.find(leak), std::string::npos) << "response leaked " << leak;
}

}  // namespace

// RTS-NF-003 / TC-ERR-01: failures return safe, understandable messages.
TEST(SafeErrorsApi, UnknownRouteReturnsGenericJson) {
    TestServer s;
    auto res = s.client().Get("/api/does-not-exist");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
    expectSafeJson(res);
    EXPECT_EQ(json::parse(res->body)["error"], "Resource not found.");
}

TEST(SafeErrorsApi, UnsupportedMethodReturnsGenericJson) {
    TestServer s;
    auto res = s.client().Delete("/api/health");
    ASSERT_TRUE(res);
    EXPECT_GE(res->status, 400);
    expectSafeJson(res);
}

TEST(SafeErrorsApi, MalformedJsonIsRejectedSafely) {
    TestServer s;
    auto res = s.client().Post("/api/auth/login", "{not json", "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    expectSafeJson(res);
    EXPECT_EQ(json::parse(res->body)["error"], "Request body must be a JSON object.");
}

TEST(SafeErrorsApi, OversizedRequestBodyIsRejectedSafely) {
    TestServer s;
    std::string body(70 * 1024, 'a');
    auto res = s.client().Post("/api/auth/login", body, "application/json");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 413);
    expectSafeJson(res);
}

TEST(SafeErrorsApi, OversizedResourceIdDoesNotCrashTheServer) {
    TestServer s;
    std::string huge(100, '9');
    auto res = s.client().Get(("/api/trains/" + huge + "/schedule").c_str());
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 404);
    expectSafeJson(res);
    auto health = s.client().Get("/api/health");
    ASSERT_TRUE(health);
    EXPECT_EQ(health->status, 200);
}

TEST(SafeErrorsApi, InjectionAttemptsReturnValidationMessages) {
    TestServer s;
    auto res = s.client().Get(
        "/api/trains/search?source=%27%20OR%201%3D1%20--&destination=Beta&date=2026-11-01");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    expectSafeJson(res);
    EXPECT_TRUE(json::parse(res->body).contains("fields"));
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM users")[0][0], "0");
}
