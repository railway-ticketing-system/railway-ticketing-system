#include <gtest/gtest.h>

#include <algorithm>
#include <chrono>
#include <ctime>
#include <mutex>
#include <thread>
#include <vector>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;

namespace {

std::string dateAfter(int days) {
    std::time_t when = std::time(nullptr) + static_cast<std::time_t>(days) * 86400;
    std::tm utc = *std::gmtime(&when);
    char buffer[11];
    std::strftime(buffer, sizeof buffer, "%Y-%m-%d", &utc);
    return buffer;
}

}  // namespace

// RTS-NF-001 / TC-PERF-01: 90% of normal search and booking requests must
// complete within 3 seconds under project load.
TEST(PerformanceApi, P90SearchLatencyStaysUnderThreeSeconds) {
    TestServer s;
    test::addTrain(s.db(), "96001", "Performance Express",
                   {{"Alpha", "", "06:00", 0}, {"Beta", "09:30", "", 0}},
                   {{"SL", "Sleeper", 250, 100}, {"3A", "AC Three Tier", 700, 50}});
    std::string token = test::registerAndLogin(s, "perf@example.com");
    ASSERT_FALSE(token.empty());
    const std::string date = dateAfter(3);
    const int port = s.port();

    constexpr int kThreads = 4;
    constexpr int kRequestsPerThread = 20;
    std::vector<double> milliseconds;
    std::mutex mutex;
    std::vector<std::thread> workers;

    for (int t = 0; t < kThreads; ++t) {
        workers.emplace_back([&, port] {
            httplib::Client client("127.0.0.1", port);
            client.set_connection_timeout(5, 0);
            client.set_read_timeout(5, 0);
            std::vector<double> local;
            local.reserve(kRequestsPerThread);
            for (int i = 0; i < kRequestsPerThread; ++i) {
                auto start = std::chrono::steady_clock::now();
                auto res = client.Get(("/api/trains/search?source=Alpha&destination=Beta&date=" + date).c_str());
                auto elapsed = std::chrono::duration<double, std::milli>(
                                   std::chrono::steady_clock::now() - start)
                                   .count();
                EXPECT_TRUE(res);
                if (res) EXPECT_EQ(res->status, 200);
                local.push_back(elapsed);
            }
            std::lock_guard<std::mutex> lock(mutex);
            milliseconds.insert(milliseconds.end(), local.begin(), local.end());
        });
    }
    for (auto& worker : workers) worker.join();

    ASSERT_EQ(milliseconds.size(), static_cast<size_t>(kThreads * kRequestsPerThread));
    std::sort(milliseconds.begin(), milliseconds.end());
    double p90 = milliseconds[(milliseconds.size() * 9) / 10];
    EXPECT_LT(p90, 3000.0) << "90th percentile latency was " << p90 << " ms";
}
