#include <gtest/gtest.h>

#include <thread>

#include "api/Routes.h"

TEST(Health, ReturnsOk) {
    rtrs::Database db(":memory:");
    httplib::Server server;
    rtrs::registerRoutes(server, db);
    int port = server.bind_to_any_port("127.0.0.1");
    std::thread t([&] { server.listen_after_bind(); });
    server.wait_until_ready();

    httplib::Client cli("127.0.0.1", port);
    auto res = cli.Get("/api/health");
    ASSERT_TRUE(res);
    EXPECT_EQ(res->status, 200);
    EXPECT_NE(res->body.find("ok"), std::string::npos);

    server.stop();
    t.join();
}
