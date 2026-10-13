#include <gtest/gtest.h>

#include "util/Validation.h"

using namespace rtrs;

TEST(Validation, NameBoundaries) {
    EXPECT_FALSE(isValidName("A"));
    EXPECT_TRUE(isValidName("Al"));
    EXPECT_TRUE(isValidName(std::string(60, 'a')));
    EXPECT_FALSE(isValidName(std::string(61, 'a')));
    EXPECT_TRUE(isValidName("Mary-Jane O'Neil Jr."));
    EXPECT_FALSE(isValidName("R2D2"));
    EXPECT_FALSE(isValidName("<script>"));
    EXPECT_FALSE(isValidName("..--"));  // no letters
}

TEST(Validation, Email) {
    EXPECT_TRUE(isValidEmail("a.b+c@example.co.in"));
    EXPECT_FALSE(isValidEmail("plainaddress"));
    EXPECT_FALSE(isValidEmail("@example.com"));
    EXPECT_FALSE(isValidEmail("a@b"));
    EXPECT_FALSE(isValidEmail("a b@example.com"));
    EXPECT_FALSE(isValidEmail("a@example.com' OR '1'='1"));
    EXPECT_FALSE(isValidEmail(std::string(250, 'a') + "@x.com"));
}

TEST(Validation, Phone) {
    EXPECT_TRUE(isValidPhone("9876543210"));
    EXPECT_FALSE(isValidPhone("5876543210"));  // must start 6-9
    EXPECT_FALSE(isValidPhone("987654321"));   // 9 digits
    EXPECT_FALSE(isValidPhone("98765432100")); // 11 digits
    EXPECT_FALSE(isValidPhone("98765abcde"));
}

TEST(Validation, Password) {
    EXPECT_TRUE(isValidPassword("abcdefg1"));
    EXPECT_FALSE(isValidPassword("abcdef1"));     // 7 chars
    EXPECT_FALSE(isValidPassword("abcdefgh"));    // no digit
    EXPECT_FALSE(isValidPassword("12345678"));    // no letter
    EXPECT_TRUE(isValidPassword(std::string(127, 'a') + "1"));
    EXPECT_FALSE(isValidPassword(std::string(128, 'a') + "1"));
}

TEST(Validation, RegistrationReportsEveryBadField) {
    auto errs = validateRegistration({"", "bad", "123", "x"});
    EXPECT_EQ(errs.size(), 4u);
    EXPECT_TRUE(validateRegistration({"Asha Rao", "asha@example.com", "9876543210", "secret123"}).empty());
}

TEST(Validation, Trim) {
    EXPECT_EQ(trim("  hi \t\n"), "hi");
    EXPECT_EQ(trim("   "), "");
}
