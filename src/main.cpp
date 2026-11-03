#include <cstdlib>
#include <iostream>
#include <memory>

#include "api/Routes.h"
#include "db/Database.h"
#include "db/Schema.h"
#include "services/UserService.h"
#include "util/TlsConfig.h"

int main(int argc, char* argv[]) {
    const char* dbPath = std::getenv("RTRS_DB");
    const char* portEnv = std::getenv("RTRS_PORT");
    int port = portEnv ? std::atoi(portEnv) : 8080;

    try {
        rtrs::Database db(dbPath ? dbPath : "rtrs.db");
        if (argc != 1) {
            if (argc != 3 || std::string(argv[1]) != "--promote-admin") {
                std::cerr << "Usage: rtrs_server [--promote-admin EMAIL]" << std::endl;
                return 1;
            }
            rtrs::initSchema(db);
            rtrs::UserService users(db, rtrs::PasswordHasher{});
            auto account = users.findCredentials(argv[2]);
            if (!account) {
                std::cerr << "Account not found. Register the administrator's account first." << std::endl;
                return 1;
            }
            db.transaction([&] {
                db.execute("UPDATE users SET role = 'ADMIN' WHERE id = ?", {std::to_string(account->first.id)});
                db.execute("DELETE FROM sessions WHERE user_id = ?", {std::to_string(account->first.id)});
            });
            std::cout << "Administrator access granted. Log in again to continue." << std::endl;
            return 0;
        }
        // RTS-SR-003: serve HTTPS when a certificate and key are configured.
        rtrs::TlsConfig tls = rtrs::tlsConfigFromEnvironment();
        std::string tlsError;
        if (!tls.valid(tlsError)) {
            std::cerr << "TLS configuration error: " << tlsError << std::endl;
            return 1;
        }
        std::unique_ptr<httplib::Server> server;
        if (tls.enabled())
            server = std::make_unique<httplib::SSLServer>(tls.certPath.c_str(), tls.keyPath.c_str());
        else
            server = std::make_unique<httplib::Server>();
        if (!server->is_valid()) {
            std::cerr << "Could not initialize the server (check the TLS certificate and key)." << std::endl;
            return 1;
        }
        server->set_mount_point("/", std::getenv("RTRS_WEB_DIR") ? std::getenv("RTRS_WEB_DIR") : "web");

        rtrs::CancellationPolicy policy;
        if (const char* cutoff = std::getenv("RTRS_CANCEL_CUTOFF_HOURS")) {
            int hours = std::atoi(cutoff);
            if (hours >= 0) policy.cutoffHours = hours;
        }
        if (const char* refund = std::getenv("RTRS_REFUND_PERCENT")) {
            int percent = std::atoi(refund);
            if (percent >= 0 && percent <= 100) policy.refundPercent = percent;
        }
        int iterations = rtrs::PasswordHasher::kDefaultIterations;
        if (const char* configured = std::getenv("RTRS_PBKDF2_ITERATIONS")) {
            int value = std::atoi(configured);
            if (value >= rtrs::PasswordHasher::kMinIterations && value <= rtrs::PasswordHasher::kMaxIterations)
                iterations = value;
            else
                std::cerr << "Ignoring RTRS_PBKDF2_ITERATIONS outside "
                          << rtrs::PasswordHasher::kMinIterations << "-"
                          << rtrs::PasswordHasher::kMaxIterations << std::endl;
        }
        rtrs::registerRoutes(*server, db, iterations, policy);
        std::cout << "RTRS listening on " << (tls.enabled() ? "https" : "http") << "://0.0.0.0:" << port
                  << std::endl;
        if (!server->listen("0.0.0.0", port)) {
            std::cerr << "failed to bind port " << port << std::endl;
            return 1;
        }
    } catch (const std::exception& e) {
        std::cerr << "fatal: " << e.what() << std::endl;
        return 1;
    }
    return 0;
}
