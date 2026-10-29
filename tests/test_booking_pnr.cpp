#include <gtest/gtest.h>

#include <algorithm>
#include <cctype>
#include <set>
#include <nlohmann/json.hpp>

#include "ApiHelpers.h"
#include "TestServer.h"
#include "TrainFixtures.h"

using namespace rtrs;
using nlohmann::json;

namespace {

long long addBookingTrain(TestServer& s) {
    return test::addTrain(s.db(), "93005", "Pnr Express",
                          {{"Alpha Jn", "", "06:00", 0}, {"Beta Jn", "09:30", "", 0}},
                          {{"SL", "Sleeper", 250, 50}});
}

json confirmBooking(TestServer& s, const std::string& token, const json& body) {
    auto res = s.client().Post("/api/bookings", test::bearer(token), body.dump(), "application/json");
    if (!res || res->status != 201) return json::object();
    return json::parse(res->body)["booking"];
}

bool isTenDigitPnr(const std::string& pnr) {
    return pnr.size() == 10 && pnr[0] != '0' &&
           std::all_of(pnr.begin(), pnr.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
}

}  // namespace

TEST(BookingPnrApi, ReturnsTenDigitPnr) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);

    auto booking = confirmBooking(s, token, test::bookingBody(trainId));

    ASSERT_FALSE(booking.empty());
    EXPECT_TRUE(isTenDigitPnr(booking["pnr"]));
}

TEST(BookingPnrApi, PnrIsStoredOnTheBooking) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);

    auto booking = confirmBooking(s, token, test::bookingBody(trainId));

    ASSERT_FALSE(booking.empty());
    auto rows = s.db().query("SELECT pnr FROM bookings WHERE id = ?",
                             {std::to_string(booking["id"].get<long long>())});
    ASSERT_EQ(rows.size(), 1u);
    EXPECT_EQ(rows[0][0], booking["pnr"].get<std::string>());
}

TEST(BookingPnrApi, PnrIsUniqueAcrossBookings) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);
    std::set<std::string> pnrs;

    for (int i = 0; i < 25; ++i) {
        auto booking = confirmBooking(s, token, test::bookingBody(trainId));
        ASSERT_FALSE(booking.empty());
        std::string pnr = booking["pnr"];
        EXPECT_TRUE(isTenDigitPnr(pnr));
        EXPECT_TRUE(pnrs.insert(pnr).second) << "duplicate PNR " << pnr;
    }
    EXPECT_EQ(pnrs.size(), 25u);
}

TEST(BookingPnrApi, PnrDoesNotLeakSequentialIds) {
    TestServer s;
    long long trainId = addBookingTrain(s);
    std::string token = test::registerAndLogin(s);

    auto first = confirmBooking(s, token, test::bookingBody(trainId));
    auto second = confirmBooking(s, token, test::bookingBody(trainId));

    ASSERT_FALSE(first.empty());
    ASSERT_FALSE(second.empty());
    EXPECT_NE(std::stoll(first["pnr"].get<std::string>()), first["id"].get<long long>() + 1000000000LL);
    EXPECT_NE(std::stoll(second["pnr"].get<std::string>()), second["id"].get<long long>() + 1000000000LL);
}
