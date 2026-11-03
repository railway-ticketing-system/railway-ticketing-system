#include <gtest/gtest.h>

#include <atomic>
#include <ctime>
#include <mutex>
#include <set>
#include <thread>
#include <vector>

#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"
#include "db/Schema.h"

using namespace rtrs;
using nlohmann::json;

namespace {

std::string dateAfter(int days) {
    std::time_t when = std::time(nullptr) + static_cast<std::time_t>(days) * 86400;
    std::tm utc = *std::gmtime(&when);
    char buffer[11];
    std::strftime(buffer, sizeof buffer, "%Y-%m-%d", &utc);
    return buffer;
}

struct Attempt {
    int status = 0;
    std::string seat;
};

}  // namespace

// RTS-NF-002 / TC-CONC-01: concurrent confirmations must never allocate the
// same seat twice or overbook the class.
TEST(ConcurrencyApi, ConcurrentBookingsAllocateEachSeatOnlyOnce) {
    TestServer s;
    const long long trainId = test::addTrain(s.db(), "97001", "Concurrency Express",
                                             {{"Alpha", "", "06:00", 0}, {"Beta", "09:30", "", 0}},
                                             {{"SL", "Sleeper", 250, 2}});
    const std::string date = dateAfter(3);
    const int port = s.port();

    constexpr int kBuyers = 8;
    std::vector<std::string> tokens;
    for (int i = 0; i < kBuyers; ++i)
        tokens.push_back(test::registerAndLogin(s, "buyer" + std::to_string(i) + "@example.com", "Asha Rao"));
    for (const auto& token : tokens) ASSERT_FALSE(token.empty());

    std::atomic<int> ready{0};
    std::atomic<bool> go{false};
    std::mutex mutex;
    std::vector<Attempt> attempts;
    std::vector<std::thread> workers;

    for (int i = 0; i < kBuyers; ++i) {
        workers.emplace_back([&, i] {
            httplib::Client client("127.0.0.1", port);
            client.set_read_timeout(10, 0);
            ready.fetch_add(1);
            while (!go.load()) std::this_thread::yield();
            auto res = client.Post("/api/bookings", test::bearer(tokens[i]),
                                   test::bookingBody(trainId, "SL", {"Asha Rao"}, date).dump(),
                                   "application/json");
            Attempt attempt;
            if (res) {
                attempt.status = res->status;
                if (res->status == 201) {
                    auto booking = json::parse(res->body)["booking"];
                    attempt.seat = booking["passengers"][0]["seatNumber"].get<std::string>();
                }
            }
            std::lock_guard<std::mutex> lock(mutex);
            attempts.push_back(attempt);
        });
    }
    while (ready.load() < kBuyers) std::this_thread::yield();
    go.store(true);
    for (auto& worker : workers) worker.join();

    ASSERT_EQ(attempts.size(), static_cast<size_t>(kBuyers));
    int created = 0, rejected = 0;
    std::set<std::string> seats;
    for (const auto& attempt : attempts) {
        if (attempt.status == 201) {
            ++created;
            seats.insert(attempt.seat);
        } else if (attempt.status == 409) {
            ++rejected;
        }
    }
    EXPECT_EQ(created, 2);
    EXPECT_EQ(rejected, kBuyers - 2);
    EXPECT_EQ(seats.size(), 2u);
    EXPECT_EQ(s.db().query("SELECT COUNT(*) FROM passengers")[0][0], "2");
    EXPECT_EQ(s.db().query("SELECT COUNT(DISTINCT seat_number) FROM passengers")[0][0], "2");
    EXPECT_EQ(s.db().query("SELECT SUM(seats) FROM seat_reservations")[0][0], "2");
}

TEST(ConcurrencyDatabase, UniqueSeatTriggerRejectsDirectDuplicates) {
    Database db(":memory:");
    initSchema(db);
    long long trainId = test::addTrain(db, "97002", "Trigger Express",
                                       {{"Alpha", "", "06:00", 0}, {"Beta", "09:30", "", 0}},
                                       {{"SL", "Sleeper", 250, 5}});
    db.execute("INSERT INTO users (name, email, phone, password_hash) VALUES (?, ?, ?, ?)",
               {"Asha Rao", "trigger@example.com", "9876543210", "pbkdf2$1$00$00"});
    long long userId = db.lastInsertId();
    auto booking = [&](const std::string& pnr) {
        db.execute(
            "INSERT INTO bookings (pnr, user_id, train_id, travel_date, class_code, fare, passenger_count)"
            " VALUES (?, ?, ?, ?, ?, ?, ?)",
            {pnr, std::to_string(userId), std::to_string(trainId), "2026-12-01", "SL", "250", "1"});
        return db.lastInsertId();
    };
    long long first = booking("8000000001");
    db.execute("INSERT INTO passengers (booking_id, name, age, gender, seat_number) VALUES (?, ?, ?, ?, ?)",
               {std::to_string(first), "Asha Rao", "30", "M", "SL-1"});
    EXPECT_EQ(db.query("SELECT COUNT(*) FROM passengers")[0][0], "1");

    long long second = booking("8000000002");
    EXPECT_THROW(db.execute(
                     "INSERT INTO passengers (booking_id, name, age, gender, seat_number) VALUES (?, ?, ?, ?, ?)",
                     {std::to_string(second), "Ravi Kumar", "31", "M", "SL-1"}),
                 std::runtime_error);
    EXPECT_EQ(db.query("SELECT COUNT(*) FROM passengers")[0][0], "1");
}
