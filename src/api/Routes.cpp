#include "api/Routes.h"

#include <nlohmann/json.hpp>

namespace rtrs {

void registerRoutes(httplib::Server& server, Database& /*db*/) {
    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        res.set_content(nlohmann::json{{"status", "ok"}}.dump(), "application/json");
    });
}

}  // namespace rtrs
