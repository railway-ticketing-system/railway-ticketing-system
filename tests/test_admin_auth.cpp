#include <gtest/gtest.h>
#include "AdminHelpers.h"

using nlohmann::json;
using namespace rtrs;

TEST(AdminAuthApi, AuthorizedCredentialsReturnAdminAndAccess) {
    TestServer s;
    auto token = test::adminLogin(s);
    ASSERT_FALSE(token.empty());
    auto res = s.client().Get("/api/admin/me", test::bearer(token));
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    auto body = json::parse(res->body);
    EXPECT_EQ(body["user"]["role"], "ADMIN");
    EXPECT_EQ(res->body.find("password"), std::string::npos);
    s.db().execute("UPDATE users SET role = 'PASSENGER' WHERE email = 'admin@example.com'");
    auto demoted = s.client().Get("/api/admin/me", test::bearer(token));
    ASSERT_TRUE(demoted);
    EXPECT_EQ(demoted->status, 403);
}

TEST(AdminAuthApi, RejectsUnauthenticatedPassengerAndRevokedSessions) {
    TestServer s;
    auto missing = s.client().Get("/api/admin/me");
    ASSERT_TRUE(missing);
    EXPECT_EQ(missing->status, 401);
    auto passenger = test::registerAndLogin(s);
    auto denied = s.client().Get("/api/admin/me", test::bearer(passenger));
    ASSERT_TRUE(denied);
    EXPECT_EQ(denied->status, 403);
    auto token = test::adminLogin(s);
    s.client().Post("/api/auth/logout", test::bearer(token), "", "application/json");
    auto revoked = s.client().Get("/api/admin/me", test::bearer(token));
    ASSERT_TRUE(revoked);
    EXPECT_EQ(revoked->status, 401);
}

TEST(AdminAuthApi, PublicRegistrationCannotChooseAdminRole) {
    TestServer s;
    auto registration = s.client().Post("/api/auth/register", json{
        {"name", "Ravi Kumar"}, {"email", "forged@example.com"}, {"phone", "9876543210"},
        {"password", "secret123"}, {"role", "ADMIN"}}.dump(), "application/json");
    ASSERT_TRUE(registration);
    ASSERT_EQ(registration->status, 201);
    EXPECT_EQ(json::parse(registration->body)["user"]["role"], "PASSENGER");
    auto token = test::registerAndLogin(s, "forged@example.com");
    auto res = s.client().Get("/api/admin/me", test::bearer(token));
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 403);
}
