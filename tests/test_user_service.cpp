#include <gtest/gtest.h>

#include "db/Schema.h"
#include "services/UserService.h"

using namespace rtrs;
using Status = RegisterResult::Status;

namespace {
struct Fixture : ::testing::Test {
    Database db{":memory:"};
    UserService svc{(initSchema(db), db), PasswordHasher(1000)};
    RegistrationInput valid{"Asha Rao", "Asha@Example.com", "9876543210", "secret123"};
};
}  // namespace

TEST_F(Fixture, RegistersValidUser) {
    auto r = svc.registerUser(valid);
    ASSERT_EQ(r.status, Status::Created);
    EXPECT_GT(r.user.id, 0);
    EXPECT_EQ(r.user.email, "asha@example.com");  // normalised
    EXPECT_EQ(r.user.role, "PASSENGER");
}

TEST_F(Fixture, PasswordIsStoredHashedOnly) {
    svc.registerUser(valid);
    auto rows = db.query("SELECT password_hash FROM users");
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0][0].find("secret123"), std::string::npos);
    EXPECT_EQ(rows[0][0].rfind("pbkdf2$", 0), 0u);
}

TEST_F(Fixture, DuplicateEmailIsRejectedCaseInsensitively) {
    ASSERT_EQ(svc.registerUser(valid).status, Status::Created);
    valid.email = "ASHA@example.COM";
    auto r = svc.registerUser(valid);
    EXPECT_EQ(r.status, Status::EmailTaken);
    EXPECT_EQ(db.query("SELECT 1 FROM users").size(), 1u);
}

TEST_F(Fixture, InvalidInputCreatesNothing) {
    valid.phone = "123";
    auto r = svc.registerUser(valid);
    EXPECT_EQ(r.status, Status::ValidationFailed);
    EXPECT_TRUE(r.errors.count("phone"));
    EXPECT_TRUE(db.query("SELECT 1 FROM users").empty());
}

TEST_F(Fixture, SurroundingWhitespaceIsTrimmed) {
    valid.name = "  Asha Rao  ";
    valid.email = " asha@example.com ";
    auto r = svc.registerUser(valid);
    ASSERT_EQ(r.status, Status::Created);
    EXPECT_EQ(r.user.name, "Asha Rao");
}

TEST_F(Fixture, SqlInjectionInNameIsRejectedAndTableSurvives) {
    valid.name = "x'); DROP TABLE users;--";
    EXPECT_EQ(svc.registerUser(valid).status, Status::ValidationFailed);
    EXPECT_NO_THROW(db.query("SELECT * FROM users"));
}
