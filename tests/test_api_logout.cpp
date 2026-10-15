#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "TestServer.h"

using nlohmann::json;

namespace {
std::string loginToken(TestServer& s) {
    s.client().Post("/api/auth/register",
                    R"({"name":"Asha Rao","email":"asha@example.com","phone":"9876543210","password":"secret123"})",
                    "application/json");
    auto r = s.client().Post("/api/auth/login", R"({"email":"asha@example.com","password":"secret123"})",
                             "application/json");
    return json::parse(r->body)["token"];
}
httplib::Result logout(TestServer& s, const std::string& token) {
    httplib::Headers h;
    if (!token.empty()) h.emplace("Authorization", "Bearer " + token);
    return s.client().Post("/api/auth/logout", h, "", "application/json");
}
}  // namespace

TEST(LogoutApi, LogsOutAndTokenStopsWorking) {
    TestServer s;
    auto token = loginToken(s);
    EXPECT_EQ(logout(s, token)->status, 200);
    EXPECT_EQ(logout(s, token)->status, 401);  // already ended
    EXPECT_TRUE(s.db().query("SELECT 1 FROM sessions").empty());
}

TEST(LogoutApi, RequiresBearerToken) {
    TestServer s;
    EXPECT_EQ(logout(s, "")->status, 401);
    EXPECT_EQ(logout(s, "not-a-real-token")->status, 401);
    httplib::Headers wrongScheme{{"Authorization", "Basic abc"}};
    EXPECT_EQ(s.client().Post("/api/auth/logout", wrongScheme, "", "application/json")->status, 401);
}
