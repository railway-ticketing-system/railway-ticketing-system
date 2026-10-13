#include <gtest/gtest.h>

#include "auth/PasswordHasher.h"

using rtrs::PasswordHasher;

TEST(PasswordHasher, HashVerifiesAndIsNotPlaintext) {
    PasswordHasher h(1000);
    auto stored = h.hash("secret123");
    EXPECT_EQ(stored.find("secret123"), std::string::npos);
    EXPECT_EQ(stored.rfind("pbkdf2$1000$", 0), 0u);
    EXPECT_TRUE(h.verify("secret123", stored));
    EXPECT_FALSE(h.verify("Secret123", stored));
}

TEST(PasswordHasher, SamePasswordGetsDifferentSalts) {
    PasswordHasher h(1000);
    EXPECT_NE(h.hash("secret123"), h.hash("secret123"));
}

TEST(PasswordHasher, MalformedStoredValueIsRejected) {
    PasswordHasher h(1000);
    EXPECT_FALSE(h.verify("x", ""));
    EXPECT_FALSE(h.verify("x", "plaintext"));
    EXPECT_FALSE(h.verify("x", "pbkdf2$abc$00$00"));
    EXPECT_FALSE(h.verify("x", "pbkdf2$0$aabb$" + std::string(64, 'a')));
    EXPECT_FALSE(h.verify("x", "pbkdf2$1000$zz$" + std::string(64, 'a')));
}

TEST(PasswordHasher, VerifyUsesStoredIterationCount) {
    auto stored = PasswordHasher(500).hash("secret123");
    EXPECT_TRUE(PasswordHasher(9999).verify("secret123", stored));
}

TEST(Random, RandomHexLengthAndUniqueness) {
    auto a = rtrs::randomHex(32), b = rtrs::randomHex(32);
    EXPECT_EQ(a.size(), 64u);
    EXPECT_NE(a, b);
}
