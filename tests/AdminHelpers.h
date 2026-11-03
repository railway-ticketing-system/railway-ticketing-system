#pragma once
#include "ApiHelpers.h"

namespace rtrs::test {

inline std::string adminLogin(TestServer& server) {
    registerAndLogin(server, "admin@example.com");
    server.db().execute("UPDATE users SET role = 'ADMIN' WHERE email = ?", {"admin@example.com"});
    auto res = server.client().Post("/api/auth/login",
        nlohmann::json{{"email", "admin@example.com"}, {"password", "secret123"}}.dump(), "application/json");
    if (!res || res->status != 200) return "";
    return nlohmann::json::parse(res->body)["token"].get<std::string>();
}

}
