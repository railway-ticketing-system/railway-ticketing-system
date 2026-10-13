#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "TestServer.h"

using nlohmann::json;

namespace {
json goodBody() {
    return {{"name", "Asha Rao"}, {"email", "asha@example.com"}, {"phone", "9876543210"}, {"password", "secret123"}};
}
httplib::Result post(TestServer& s, const std::string& body) {
    return s.client().Post("/api/auth/register", body, "application/json");
}
}  // namespace

TEST(RegisterApi, CreatesAccountAndNeverEchoesSecrets) {
    TestServer s;
    auto res = post(s, goodBody().dump());
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 201);
    auto j = json::parse(res->body);
    EXPECT_EQ(j["user"]["email"], "asha@example.com");
    EXPECT_EQ(res->body.find("secret123"), std::string::npos);
    EXPECT_EQ(res->body.find("password"), std::string::npos);
}

TEST(RegisterApi, ValidationErrorsListEachField) {
    TestServer s;
    auto res = post(s, R"({"name":"","email":"nope","phone":"1","password":"x"})");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 400);
    EXPECT_EQ(json::parse(res->body)["fields"].size(), 4u);
}

TEST(RegisterApi, DuplicateEmailReturns409) {
    TestServer s;
    ASSERT_EQ(post(s, goodBody().dump())->status, 201);
    EXPECT_EQ(post(s, goodBody().dump())->status, 409);
}

TEST(RegisterApi, MalformedAndNonObjectJsonReturn400) {
    TestServer s;
    EXPECT_EQ(post(s, "{not json")->status, 400);
    EXPECT_EQ(post(s, "[1,2]")->status, 400);
    EXPECT_EQ(post(s, "")->status, 400);
}

TEST(RegisterApi, WrongTypesAreTreatedAsMissing) {
    TestServer s;
    auto b = goodBody();
    b["phone"] = 9876543210LL;  // number instead of string
    EXPECT_EQ(post(s, b.dump())->status, 400);
}

TEST(RegisterApi, OversizedBodyIsRejected) {
    TestServer s;
    auto b = goodBody();
    b["name"] = std::string(100 * 1024, 'a');
    auto res = post(s, b.dump());
    ASSERT_TRUE(res);
    EXPECT_GE(res->status, 400);
}
