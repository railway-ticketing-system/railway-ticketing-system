#include <gtest/gtest.h>

#include <iostream>
#include <sstream>
#include <string>

#include <nlohmann/json.hpp>

#include "TestServer.h"
#include "util/Log.h"

using namespace rtrs;

TEST(LoggingUtil, RedactsAuthenticationSecrets) {
    const std::string line =
        "password=secret123 token=abc789 Authorization: Bearer xyz456 api_key=topsecret";
    std::string redacted = redactForLog(line);
    EXPECT_EQ(redacted.find("secret123"), std::string::npos);
    EXPECT_EQ(redacted.find("abc789"), std::string::npos);
    EXPECT_EQ(redacted.find("xyz456"), std::string::npos);
    EXPECT_EQ(redacted.find("topsecret"), std::string::npos);
    EXPECT_NE(redacted.find("[REDACTED]"), std::string::npos);
}

TEST(LoggingUtil, RequestTargetDropsQueryAndControlCharacters) {
    EXPECT_EQ(safeRequestTarget("/api/bookings?token=abc&email=a@b.com"), "/api/bookings");
    EXPECT_EQ(safeRequestTarget("/api/bookings#fragment"), "/api/bookings");
    EXPECT_EQ(safeRequestTarget("/api/health\nGET /forged"), "/api/healthGET /forged");
    EXPECT_EQ(safeRequestTarget(std::string(300, 'a')).size(), 200u);
}

// RTS-SR-005 / TC-SEC-06: access logs contain no authentication data.
TEST(LoggingUtil, AccessLogOmitsCredentialsAndQueryValues) {
    std::ostringstream captured;
    std::streambuf* previous = std::cerr.rdbuf(captured.rdbuf());
    logAccess("POST", "/api/auth/login?email=log@example.com&password=secret123&token=abc789", 401, "127.0.0.1");
    std::cerr.rdbuf(previous);

    const std::string log = captured.str();
    EXPECT_NE(log.find("POST /api/auth/login 401"), std::string::npos);
    EXPECT_NE(log.find("127.0.0.1"), std::string::npos);
    EXPECT_EQ(log.find("secret123"), std::string::npos);
    EXPECT_EQ(log.find("abc789"), std::string::npos);
    EXPECT_EQ(log.find("log@example.com"), std::string::npos);
}
