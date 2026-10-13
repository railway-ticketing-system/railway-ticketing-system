#include "api/Routes.h"

#include <nlohmann/json.hpp>
#include <memory>

#include "db/Schema.h"
#include "services/AuthService.h"
#include "services/UserService.h"

namespace rtrs {

using nlohmann::json;

namespace {

void sendJson(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

void sendError(httplib::Response& res, int status, const std::string& message, const FieldErrors& fields = {}) {
    json body{{"error", message}};
    if (!fields.empty()) body["fields"] = fields;
    sendJson(res, status, body);
}

// Parses the request body as a JSON object; writes a 400 and returns false otherwise.
bool parseBody(const httplib::Request& req, httplib::Response& res, json& out) {
    out = json::parse(req.body, nullptr, false);
    if (out.is_discarded() || !out.is_object()) {
        sendError(res, 400, "Request body must be a JSON object.");
        return false;
    }
    return true;
}

std::string str(const json& j, const char* key) {
    auto it = j.find(key);
    return (it != j.end() && it->is_string()) ? it->get<std::string>() : std::string();
}

json userJson(const User& u) {
    return {{"id", u.id}, {"name", u.name}, {"email", u.email}, {"phone", u.phone}, {"role", u.role},
            {"createdAt", u.createdAt}};
}

}  // namespace

void registerRoutes(httplib::Server& server, Database& db, int pbkdf2Iterations) {
    initSchema(db);
    auto users = std::make_shared<UserService>(db, PasswordHasher(pbkdf2Iterations));
    auto auth = std::make_shared<AuthService>(db, *users, PasswordHasher(pbkdf2Iterations));

    server.set_payload_max_length(64 * 1024);
    // RTS-NF-003: never leak internals in error responses.
    server.set_exception_handler([](const httplib::Request&, httplib::Response& res, std::exception_ptr) {
        sendError(res, 500, "Something went wrong. Please try again.");
    });

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        sendJson(res, 200, {{"status", "ok"}});
    });

    // RTRS-9 / RTS-F-001
    server.Post("/api/auth/register", [users](const httplib::Request& req, httplib::Response& res) {
        json body;
        if (!parseBody(req, res, body)) return;
        RegistrationInput in{str(body, "name"), str(body, "email"), str(body, "phone"), str(body, "password")};
        auto result = users->registerUser(in);
        switch (result.status) {
            case RegisterResult::Status::Created:
                sendJson(res, 201, {{"user", userJson(result.user)}});
                break;
            case RegisterResult::Status::EmailTaken:
                sendError(res, 409, "Registration failed.", result.errors);
                break;
            default:
                sendError(res, 400, "Please correct the highlighted fields.", result.errors);
        }
    });

    // RTRS-10 / RTS-F-002
    server.Post("/api/auth/login", [auth](const httplib::Request& req, httplib::Response& res) {
        json body;
        if (!parseBody(req, res, body)) return;
        auto r = auth->login(str(body, "email"), str(body, "password"));
        switch (r.status) {
            case LoginResult::Status::Ok:
                sendJson(res, 200, {{"token", r.token}, {"expiresAt", r.expiresAt}, {"user", userJson(r.user)}});
                break;
            case LoginResult::Status::Locked:
                sendError(res, 429, "Too many failed attempts. Try again in a few minutes.");
                break;
            default:  // deliberately identical for unknown email and wrong password
                sendError(res, 401, "Invalid email or password.");
        }
    });
}

}  // namespace rtrs
