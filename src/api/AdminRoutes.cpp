#include "api/AdminRoutes.h"

#include <nlohmann/json.hpp>

#include "api/Authorization.h"

namespace rtrs {

void registerAdminRoutes(httplib::Server& server, std::shared_ptr<AuthService> auth) {
    server.Get("/api/admin/me", [auth](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAdmin(*auth, req, res);
        if (!user) return;
        res.set_content(nlohmann::json{{"user", {{"id", user->id}, {"name", user->name},
                          {"email", user->email}, {"role", user->role}}}}.dump(), "application/json");
    });
}

}
