#include <cstdlib>
#include <iostream>

#include "api/Routes.h"
#include "db/Database.h"

int main() {
    const char* dbPath = std::getenv("RTRS_DB");
    const char* portEnv = std::getenv("RTRS_PORT");
    int port = portEnv ? std::atoi(portEnv) : 8080;

    try {
        rtrs::Database db(dbPath ? dbPath : "rtrs.db");
        httplib::Server server;
        server.set_mount_point("/", std::getenv("RTRS_WEB_DIR") ? std::getenv("RTRS_WEB_DIR") : "web");

        rtrs::CancellationPolicy policy;
        if (const char* cutoff = std::getenv("RTRS_CANCEL_CUTOFF_HOURS")) {
            int hours = std::atoi(cutoff);
            if (hours >= 0) policy.cutoffHours = hours;
        }
        if (const char* refund = std::getenv("RTRS_REFUND_PERCENT")) {
            int percent = std::atoi(refund);
            if (percent >= 0 && percent <= 100) policy.refundPercent = percent;
        }
        rtrs::registerRoutes(server, db, rtrs::PasswordHasher::kDefaultIterations, policy);
        std::cout << "RTRS listening on http://0.0.0.0:" << port << std::endl;
        if (!server.listen("0.0.0.0", port)) {
            std::cerr << "failed to bind port " << port << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
