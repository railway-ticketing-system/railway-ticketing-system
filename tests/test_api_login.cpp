#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "TestServer.h"

using nlohmann::json;

namespace {
void registerAsha(TestServer& s) {
    s.client().Post("/api/auth/register",
                    R"({"name":"Asha Rao","email":"asha@example.com","phone":"9876543210","password":"secret123"})",
                    "application/json");
}
httplib::Result login(TestServer& s, const std::string& email, const std::string& pw) {
    return s.client().Post("/api/auth/login", json{{"email", email}, {"password", pw}}.dump(), "application/json");
}
}  // namespace

TEST(LoginApi, SuccessReturnsTokenAndUserWithoutHash) {
    TestServer s;
    registerAsha(s);
    auto res = login(s, "asha@example.com", "secret123");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    auto j = json::parse(res->body);
    EXPECT_EQ(j["token"].get<std::string>().size(), 64u);
    EXPECT_EQ(j["user"]["email"], "asha@example.com");
    EXPECT_EQ(res->body.find("pbkdf2"), std::string::npos);
}

TEST(LoginApi, BadCredentialsReturnTheSame401Body) {
    TestServer s;
    registerAsha(s);
    auto wrongPw = login(s, "asha@example.com", "wrongpass1");
    auto noUser = login(s, "ghost@example.com", "secret123");
    EXPECT_EQ(wrongPw->status, 401);
    EXPECT_EQ(noUser->status, 401);
    EXPECT_EQ(wrongPw->body, noUser->body);
}

TEST(LoginApi, MissingFieldsAndBadJson) {
    TestServer s;
    EXPECT_EQ(login(s, "", "")->status, 401);
    EXPECT_EQ(s.client().Post("/api/auth/login", "{oops", "application/json")->status, 400);
}

TEST(LoginApi, RepeatedFailuresReturn429) {
    TestServer s;
    registerAsha(s);
    for (int i = 0; i < 5; ++i) login(s, "asha@example.com", "badpass123");
    EXPECT_EQ(login(s, "asha@example.com", "secret123")->status, 429);
}

TEST(LoginApi, SqlInjectionInEmailDoesNotAuthenticate) {
    TestServer s;
    registerAsha(s);
    EXPECT_EQ(login(s, "' OR '1'='1", "x")->status, 401);
}
