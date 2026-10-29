#pragma once
#include <nlohmann/json.hpp>

#include <string>
#include <vector>

#include "TestServer.h"

namespace rtrs::test {

inline std::string registerAndLogin(TestServer& server, const std::string& email = "booker@example.com",
                                    const std::string& name = "Asha Rao") {
    nlohmann::json registration{{"name", name}, {"email", email}, {"phone", "9876543210"},
                                {"password", "secret123"}};
    server.client().Post("/api/auth/register", registration.dump(), "application/json");
    auto res = server.client().Post("/api/auth/login",
                                    nlohmann::json{{"email", email}, {"password", "secret123"}}.dump(),
                                    "application/json");
    if (!res || res->status != 200) return "";
    return nlohmann::json::parse(res->body)["token"].get<std::string>();
}

inline httplib::Headers bearer(const std::string& token) {
    return {{"Authorization", "Bearer " + token}};
}

inline nlohmann::json passenger(const std::string& name, int age, const char* gender = "M") {
    return {{"name", name}, {"age", age}, {"gender", gender}};
}

inline nlohmann::json bookingBody(long long trainId, const std::string& travelClass = "SL",
                                  const std::vector<std::string>& names = {"Asha Rao"},
                                  const std::string& date = "2026-11-01") {
    nlohmann::json passengers = nlohmann::json::array();
    int age = 30;
    for (const auto& name : names) passengers.push_back(passenger(name, age++));
    return {{"trainId", trainId}, {"date", date}, {"class", travelClass}, {"passengers", passengers}};
}

}  // namespace rtrs::test
