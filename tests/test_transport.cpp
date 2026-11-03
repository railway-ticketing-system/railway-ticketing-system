#include <gtest/gtest.h>

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "TestServer.h"
#include "util/TlsConfig.h"

using namespace rtrs;

namespace {

void clearTlsEnvironment() {
    unsetenv("RTRS_TLS_CERT");
    unsetenv("RTRS_TLS_KEY");
}

std::string writeTemp(const std::string& name, const std::string& content) {
    std::string path = std::string(testing::TempDir()) + name;
    std::ofstream out(path);
    out << content;
    return path;
}

}  // namespace

// RTS-SR-003 / TC-SEC-04
TEST(TlsConfigTest, DisabledWhenNeitherPathIsSet) {
    clearTlsEnvironment();
    TlsConfig config = tlsConfigFromEnvironment();
    std::string error;
    EXPECT_FALSE(config.enabled());
    EXPECT_TRUE(config.valid(error));
}

TEST(TlsConfigTest, RequiresBothCertificateAndKey) {
    clearTlsEnvironment();
    setenv("RTRS_TLS_CERT", "/tmp/only-cert.pem", 1);
    TlsConfig config = tlsConfigFromEnvironment();
    std::string error;
    EXPECT_TRUE(config.enabled());
    EXPECT_FALSE(config.valid(error));
    EXPECT_FALSE(error.empty());
    clearTlsEnvironment();
}

TEST(TlsConfigTest, AcceptsReadableCertificateAndKey) {
    clearTlsEnvironment();
    std::string cert = writeTemp("rtrs-test-cert.pem", "certificate");
    std::string key = writeTemp("rtrs-test-key.pem", "key");
    setenv("RTRS_TLS_CERT", cert.c_str(), 1);
    setenv("RTRS_TLS_KEY", key.c_str(), 1);
    TlsConfig config = tlsConfigFromEnvironment();
    std::string error;
    EXPECT_TRUE(config.enabled());
    EXPECT_TRUE(config.valid(error));
    std::remove(cert.c_str());
    std::remove(key.c_str());
    clearTlsEnvironment();
}

TEST(TransportSecurityTest, AddsBaselineSecurityHeaders) {
    TestServer s;
    auto res = s.client().Get("/api/health");
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    EXPECT_EQ(res->get_header_value("X-Content-Type-Options"), "nosniff");
    EXPECT_EQ(res->get_header_value("X-Frame-Options"), "DENY");
    EXPECT_EQ(res->get_header_value("Referrer-Policy"), "no-referrer");
    EXPECT_TRUE(res->get_header_value("Strict-Transport-Security").empty());
}

TEST(TransportSecurityTest, AddsHstsWhenOriginalRequestWasHttps) {
    TestServer s;
    auto res = s.client().Get("/api/health", {{"X-Forwarded-Proto", "https"}});
    ASSERT_TRUE(res);
    ASSERT_EQ(res->status, 200);
    EXPECT_EQ(res->get_header_value("Strict-Transport-Security"), "max-age=31536000");
}
