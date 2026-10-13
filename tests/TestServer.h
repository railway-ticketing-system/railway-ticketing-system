#pragma once
#include <httplib.h>

#include <memory>
#include <thread>

#include "api/Routes.h"

// Spins up the real app on an ephemeral port with an in-memory database.
class TestServer {
public:
    TestServer() : db_(":memory:") {
        rtrs::registerRoutes(server_, db_, /*pbkdf2Iterations=*/1000);
        port_ = server_.bind_to_any_port("127.0.0.1");
        thread_ = std::thread([this] { server_.listen_after_bind(); });
        server_.wait_until_ready();
        client_ = std::make_unique<httplib::Client>("127.0.0.1", port_);
    }
    ~TestServer() {
        server_.stop();
        thread_.join();
    }
    httplib::Client& client() { return *client_; }
    rtrs::Database& db() { return db_; }

private:
    rtrs::Database db_;
    httplib::Server server_;
    int port_ = 0;
    std::thread thread_;
    std::unique_ptr<httplib::Client> client_;
};
