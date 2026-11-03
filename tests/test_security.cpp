#include <gtest/gtest.h>

#include <algorithm>
#include <string>

#include <nlohmann/json.hpp>

#include "AdminHelpers.h"
#include "ApiHelpers.h"
#include "TestServer.h"
#include "auth/PasswordHasher.h"

using namespace rtrs;
using nlohmann::json;

namespace {

std::string login(TestServer& s, const std::string& email, const std::string& password) {
    auto res = s.client().Post("/api/auth/login",
                               json{{"email", email}, {"password", password}}.dump(), "application/json");
    if (!res || res->status != 200) return "";
    return json::parse(res->body)["token"].get<std::string>();
}

}  // namespace

// RTS-NF-005 / RTS-SR-001 / TC-SEC-01
TEST(SecurityApi, PasswordsAreStoredOnlyAsSaltedHashes) {
    TestServer s;
    test::registerAndLogin(s, "hash@example.com");
    auto rows = s.db().query("SELECT password_hash FROM users WHERE email = ?", {"hash@example.com"});
    ASSERT_EQ(rows.size(), 1u);
    const std::string& stored = rows[0][0];
    EXPECT_EQ(stored.rfind("pbkdf2$", 0), 0u);
    EXPECT_EQ(stored.find("secret123"), std::string::npos);
    EXPECT_EQ(std::count(stored.begin(), stored.end(), '$'), 3);
}

TEST(SecurityApi, SessionTokensAreStoredOnlyAsHashes) {
    TestServer s;
    std::string token = test::registerAndLogin(s, "session@example.com");
    ASSERT_FALSE(token.empty());
    auto rows = s.db().query("SELECT token_hash FROM sessions");
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_NE(rows[0][0], token);
    EXPECT_EQ(rows[0][0], sha256Hex(token));
}

// RTS-NF-006 / RTS-SR-002 / TC-SEC-02
TEST(SecurityApi, AdminRoleIsRecheckedOnEveryRequest) {
    TestServer s;
    std::string admin = test::adminLogin(s);
    auto allowed = s.client().Get("/api/admin/me", test::bearer(admin));
    ASSERT_TRUE(allowed);
    EXPECT_EQ(allowed->status, 200);

    s.db().execute("UPDATE users SET role = 'PASSENGER' WHERE email = ?", {"admin@example.com"});
    auto denied = s.client().Get("/api/admin/me", test::bearer(admin));
    ASSERT_TRUE(denied);
    EXPECT_EQ(denied->status, 403);
}

TEST(SecurityApi, PassengerCannotUseAdminOperations) {
    TestServer s;
    std::string token = test::registerAndLogin(s, "plain@example.com");
    auto list = s.client().Get("/api/admin/trains", test::bearer(token));
    ASSERT_TRUE(list);
    EXPECT_EQ(list->status, 403);
    auto audit = s.client().Get("/api/admin/audit", test::bearer(token));
    ASSERT_TRUE(audit);
    EXPECT_EQ(audit->status, 403);
}

TEST(SecurityApi, PasswordChangeRevokesOtherSessions) {
    TestServer s;
    test::registerAndLogin(s, "rotate@example.com");
    std::string first = login(s, "rotate@example.com", "secret123");
    std::string second = login(s, "rotate@example.com", "secret123");
    ASSERT_FALSE(first.empty());
    ASSERT_FALSE(second.empty());

    auto changed = s.client().Put("/api/users/me/password", test::bearer(first),
                                  json{{"currentPassword", "secret123"}, {"newPassword", "newsecret123"}}.dump(),
                                  "application/json");
    ASSERT_TRUE(changed);
    ASSERT_EQ(changed->status, 200);

    auto current = s.client().Get("/api/users/me", test::bearer(first));
    ASSERT_TRUE(current);
    EXPECT_EQ(current->status, 200);
    auto revoked = s.client().Get("/api/users/me", test::bearer(second));
    ASSERT_TRUE(revoked);
    EXPECT_EQ(revoked->status, 401);

    auto oldPassword = s.client().Post("/api/auth/login",
                                       json{{"email", "rotate@example.com"}, {"password", "secret123"}}.dump(),
                                       "application/json");
    ASSERT_TRUE(oldPassword);
    EXPECT_EQ(oldPassword->status, 401);
}

TEST(SecurityApi, LoginUpgradesWeakerStoredHashes) {
    TestServer s;
    std::string weak = PasswordHasher(500).hash("secret123");
    s.db().execute("INSERT INTO users (name, email, phone, password_hash) VALUES (?, ?, ?, ?)",
                   {"Asha Rao", "legacy@example.com", "9876543210", weak});
    EXPECT_TRUE(login(s, "legacy@example.com", "secret123").size() > 0);
    auto rows = s.db().query("SELECT password_hash FROM users WHERE email = ?", {"legacy@example.com"});
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(PasswordHasher(1000).storedIterations(rows[0][0]), 1000);
    EXPECT_FALSE(PasswordHasher(1000).needsRehash(rows[0][0]));
}

TEST(PasswordHasherSecurity, RejectsImplausibleIterationCounts) {
    PasswordHasher hasher(1000);
    EXPECT_FALSE(hasher.verify("secret123", "pbkdf2$999999999$00$00"));
    EXPECT_FALSE(hasher.verify("secret123", "pbkdf2$0$00$00"));
    EXPECT_FALSE(hasher.verify("secret123", "not-a-hash"));
}
