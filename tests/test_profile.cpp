#include <gtest/gtest.h>

#include <nlohmann/json.hpp>

#include "TestServer.h"
#include "db/Schema.h"
#include "services/UserService.h"

using nlohmann::json;
using namespace rtrs;

// ---- service level ----
namespace {
struct ProfileServiceTest : ::testing::Test {
    Database db{":memory:"};
    UserService svc{(initSchema(db), db), PasswordHasher(1000)};
    long long id = svc.registerUser({"Asha Rao", "asha@example.com", "9876543210", "secret123"}).user.id;
};
}  // namespace

TEST_F(ProfileServiceTest, UpdatesNameAndPhoneButNotEmail) {
    auto r = svc.updateProfile(id, {"  Asha R. ", "9123456780"});
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.user.name, "Asha R.");
    EXPECT_EQ(r.user.phone, "9123456780");
    EXPECT_EQ(r.user.email, "asha@example.com");
}

TEST_F(ProfileServiceTest, InvalidUpdateChangesNothing) {
    auto r = svc.updateProfile(id, {"", "123"});
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.errors.size(), 2u);
    EXPECT_EQ(svc.findById(id)->name, "Asha Rao");
}

TEST_F(ProfileServiceTest, ChangePasswordRequiresCurrentPassword) {
    EXPECT_TRUE(svc.changePassword(id, "wrong12345", "newpass123").errors.count("currentPassword"));
    EXPECT_TRUE(svc.changePassword(id, "secret123", "short1").errors.count("newPassword"));
    auto ok = svc.changePassword(id, "secret123", "newpass123");
    EXPECT_TRUE(ok.ok);
    EXPECT_TRUE(svc.changePassword(id, "newpass123", "another456").ok);  // old password no longer needed
    EXPECT_FALSE(svc.changePassword(id, "secret123", "third7890").ok);   // old password rejected
}

TEST_F(ProfileServiceTest, UnknownUserIsReported) {
    EXPECT_FALSE(svc.updateProfile(9999, {"Valid Name", "9876543210"}).ok);
    EXPECT_FALSE(svc.changePassword(9999, "a1234567", "b1234567").ok);
}

// ---- API level ----
namespace {
struct ProfileApiTest : ::testing::Test {
    TestServer s;
    std::string register_(const std::string& email, const std::string& phone) {
        s.client().Post("/api/auth/register",
                        json{{"name", "Asha Rao"}, {"email", email}, {"phone", phone}, {"password", "secret123"}}.dump(),
                        "application/json");
        auto r = s.client().Post("/api/auth/login", json{{"email", email}, {"password", "secret123"}}.dump(),
                                 "application/json");
        return json::parse(r->body)["token"];
    }
    httplib::Headers auth(const std::string& t) { return {{"Authorization", "Bearer " + t}}; }
};
}  // namespace

TEST_F(ProfileApiTest, GetReturnsOwnProfileOnly) {
    auto a = register_("a@example.com", "9876543210");
    auto b = register_("b@example.com", "9876543211");
    EXPECT_EQ(json::parse(s.client().Get("/api/users/me", auth(a))->body)["user"]["email"], "a@example.com");
    EXPECT_EQ(json::parse(s.client().Get("/api/users/me", auth(b))->body)["user"]["email"], "b@example.com");
}

TEST_F(ProfileApiTest, RequiresLogin) {
    EXPECT_EQ(s.client().Get("/api/users/me")->status, 401);
    EXPECT_EQ(s.client().Put("/api/users/me", "{}", "application/json")->status, 401);
    EXPECT_EQ(s.client().Put("/api/users/me/password", "{}", "application/json")->status, 401);
}

TEST_F(ProfileApiTest, UpdateProfile) {
    auto t = register_("a@example.com", "9876543210");
    auto ok = s.client().Put("/api/users/me", auth(t), R"({"name":"New Name","phone":"9000000001"})", "application/json");
    EXPECT_EQ(ok->status, 200);
    EXPECT_EQ(json::parse(s.client().Get("/api/users/me", auth(t))->body)["user"]["name"], "New Name");
    auto bad = s.client().Put("/api/users/me", auth(t), R"({"name":"X","phone":"abc"})", "application/json");
    EXPECT_EQ(bad->status, 400);
}

TEST_F(ProfileApiTest, EmailCannotBeChangedThroughProfileUpdate) {
    auto t = register_("a@example.com", "9876543210");
    s.client().Put("/api/users/me", auth(t), R"({"name":"New Name","phone":"9000000001","email":"evil@example.com"})",
                   "application/json");
    EXPECT_EQ(json::parse(s.client().Get("/api/users/me", auth(t))->body)["user"]["email"], "a@example.com");
}

TEST_F(ProfileApiTest, PasswordChangeKeepsCurrentSessionAndRevokesOthers) {
    auto t1 = register_("a@example.com", "9876543210");
    auto r2 = s.client().Post("/api/auth/login", R"({"email":"a@example.com","password":"secret123"})", "application/json");
    std::string t2 = json::parse(r2->body)["token"];
    auto ch = s.client().Put("/api/users/me/password", auth(t1),
                             R"({"currentPassword":"secret123","newPassword":"newpass123"})", "application/json");
    EXPECT_EQ(ch->status, 200);
    EXPECT_EQ(s.client().Get("/api/users/me", auth(t1))->status, 200);
    EXPECT_EQ(s.client().Get("/api/users/me", auth(t2))->status, 401);
    EXPECT_EQ(s.client().Post("/api/auth/login", R"({"email":"a@example.com","password":"newpass123"})", "application/json")->status, 200);
    EXPECT_EQ(s.client().Post("/api/auth/login", R"({"email":"a@example.com","password":"secret123"})", "application/json")->status, 401);
}

TEST_F(ProfileApiTest, WrongCurrentPasswordIsRejected) {
    auto t = register_("a@example.com", "9876543210");
    auto r = s.client().Put("/api/users/me/password", auth(t),
                            R"({"currentPassword":"nope12345","newPassword":"newpass123"})", "application/json");
    EXPECT_EQ(r->status, 400);
}
