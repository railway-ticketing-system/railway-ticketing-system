#pragma once
#include "ApiHelpers.h"

namespace rtrs::test {

inline nlohmann::json managedTrainBody(const std::string& number = "94001") {
    return {{"number", number}, {"name", "Admin Night Express"},
        {"stops", nlohmann::json::array({
            {{"station", "Cedar Jn"}, {"arrival", ""}, {"departure", "22:00"}, {"dayOffset", 0}},
            {{"station", "Birch Jn"}, {"arrival", "23:00"}, {"departure", "23:10"}, {"dayOffset", 0}},
            {{"station", "Maple Jn"}, {"arrival", "02:00"}, {"departure", ""}, {"dayOffset", 1}}})},
        {"classes", nlohmann::json::array({
            {{"code", "SL"}, {"name", "Sleeper"}, {"fare", 250}, {"totalSeats", 10}},
            {{"code", "3A"}, {"name", "AC Three Tier"}, {"fare", 700}, {"totalSeats", 4}}})}};
}

inline std::string adminLogin(TestServer& server) {
    registerAndLogin(server, "admin@example.com");
    server.db().execute("UPDATE users SET role = 'ADMIN' WHERE email = ?", {"admin@example.com"});
    auto res = server.client().Post("/api/auth/login",
        nlohmann::json{{"email", "admin@example.com"}, {"password", "secret123"}}.dump(), "application/json");
    if (!res || res->status != 200) return "";
    return nlohmann::json::parse(res->body)["token"].get<std::string>();
}

}
