#include <gtest/gtest.h>

#include "db/Schema.h"
#include "services/AuthService.h"

using namespace rtrs;
using Status = LoginResult::Status;

namespace {
struct AuthServiceTest : ::testing::Test {
    Database db{":memory:"};
    UserService users{(initSchema(db), db), PasswordHasher(1000)};
    long long now = 1'000'000;
    AuthService auth{db, users, PasswordHasher(1000), [this] { return now; }};
    AuthServiceTest() {
        RegistrationInput asha{"Asha Rao", "asha@example.com", "9876543210", "secret123"};
        // cppcheck false positive: UserService copies its PasswordHasher and references the member db.
        // cppcheck-suppress danglingTemporaryLifetime
        users.registerUser(asha);
    }
};
}  // namespace

TEST_F(AuthServiceTest, ValidCredentialsOpenASession) {
    auto r = auth.login("asha@example.com", "secret123");
    ASSERT_EQ(r.status, Status::Ok);
    EXPECT_EQ(r.token.size(), 64u);
    EXPECT_EQ(r.expiresAt, now + AuthService::kSessionTtl);
    EXPECT_EQ(auth.authenticate(r.token)->email, "asha@example.com");
}

TEST_F(AuthServiceTest, EmailIsCaseAndWhitespaceInsensitive) {
    EXPECT_EQ(auth.login("  ASHA@Example.com ", "secret123").status, Status::Ok);
}

TEST_F(AuthServiceTest, WrongPasswordAndUnknownEmailLookIdentical) {
    EXPECT_EQ(auth.login("asha@example.com", "wrongpass1").status, Status::InvalidCredentials);
    EXPECT_EQ(auth.login("nobody@example.com", "secret123").status, Status::InvalidCredentials);
}

TEST_F(AuthServiceTest, RawTokenIsNotStoredInDatabase) {
    auto r = auth.login("asha@example.com", "secret123");
    auto rows = db.query("SELECT token_hash FROM sessions");
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_NE(rows[0][0], r.token);
    EXPECT_EQ(rows[0][0], sha256Hex(r.token));
}

TEST_F(AuthServiceTest, SessionExpires) {
    auto r = auth.login("asha@example.com", "secret123");
    now += AuthService::kSessionTtl - 1;
    EXPECT_TRUE(auth.authenticate(r.token));
    now += 1;
    EXPECT_FALSE(auth.authenticate(r.token));
    EXPECT_TRUE(db.query("SELECT 1 FROM sessions").empty());  // expired row is purged
}

TEST_F(AuthServiceTest, UnknownOrBlankTokenIsRejected) {
    EXPECT_FALSE(auth.authenticate(""));
    EXPECT_FALSE(auth.authenticate("deadbeef"));
    EXPECT_FALSE(auth.authenticate(std::string(1000, 'a')));
}

TEST_F(AuthServiceTest, LocksAfterFiveFailuresEvenWithCorrectPassword) {
    for (int i = 0; i < AuthService::kMaxFailures; ++i)
        EXPECT_EQ(auth.login("asha@example.com", "badpass123").status, Status::InvalidCredentials);
    EXPECT_EQ(auth.login("asha@example.com", "secret123").status, Status::Locked);
}

TEST_F(AuthServiceTest, LockExpiresAndSuccessResetsCounter) {
    for (int i = 0; i < AuthService::kMaxFailures; ++i) auth.login("asha@example.com", "badpass123");
    now += AuthService::kLockSeconds;
    EXPECT_EQ(auth.login("asha@example.com", "secret123").status, Status::Ok);
    // counter was reset: four more failures must not lock
    for (int i = 0; i < AuthService::kMaxFailures - 1; ++i) auth.login("asha@example.com", "badpass123");
    EXPECT_EQ(auth.login("asha@example.com", "secret123").status, Status::Ok);
}
