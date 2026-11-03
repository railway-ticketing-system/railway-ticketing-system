#pragma once
#include <httplib.h>
#include <nlohmann/json.hpp>

#include "services/AuthService.h"

namespace rtrs {

inline std::string bearerToken(const httplib::Request& req) {
    const std::string prefix = "Bearer ";
    std::string header = req.get_header_value("Authorization");
    return header.rfind(prefix, 0) == 0 ? header.substr(prefix.size()) : std::string();
}

inline std::optional<User> requireUser(AuthService& auth, const httplib::Request& req, httplib::Response& res) {
    auto user = auth.authenticate(bearerToken(req));
    if (!user) {
        res.status = 401;
        res.set_content(nlohmann::json{{"error", "You are not logged in."}}.dump(), "application/json");
    }
    return user;
}

inline std::optional<User> requireAdmin(AuthService& auth, const httplib::Request& req, httplib::Response& res) {
    auto user = requireUser(auth, req, res);
    if (user && user->role != "ADMIN") {
        res.status = 403;
        res.set_content(nlohmann::json{{"error", "Administrator access is required."}}.dump(), "application/json");
        return std::nullopt;
    }
    return user;
}

}
