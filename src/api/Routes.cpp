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

// Extracts the token from "Authorization: Bearer <token>"; empty if absent or malformed.
std::string bearerToken(const httplib::Request& req) {
    const std::string prefix = "Bearer ";
    std::string h = req.get_header_value("Authorization");
    return h.rfind(prefix, 0) == 0 ? h.substr(prefix.size()) : std::string();
}

// Resolves the caller from the bearer token; writes a 401 and returns nullopt if not logged in.
std::optional<User> requireUser(AuthService& auth, const httplib::Request& req, httplib::Response& res) {
    auto u = auth.authenticate(bearerToken(req));
    if (!u) sendError(res, 401, "You are not logged in.");
    return u;
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

    // RTRS-11
    server.Post("/api/auth/logout", [auth](const httplib::Request& req, httplib::Response& res) {
        if (!auth->logout(bearerToken(req))) {
            sendError(res, 401, "You are not logged in.");
            return;
        }
        sendJson(res, 200, {{"message", "Logged out."}});
    });

    // RTRS-12: view / edit own profile. The user id always comes from the session, never from the request,
    // so one passenger can never read or change another's account.
    server.Get("/api/users/me", [auth](const httplib::Request& req, httplib::Response& res) {
        auto u = requireUser(*auth, req, res);
        if (!u) return;
        sendJson(res, 200, {{"user", userJson(*u)}});
    });

    server.Put("/api/users/me", [auth, users](const httplib::Request& req, httplib::Response& res) {
        auto u = requireUser(*auth, req, res);
        if (!u) return;
        json body;
        if (!parseBody(req, res, body)) return;
        auto r = users->updateProfile(u->id, {str(body, "name"), str(body, "phone")});
        if (!r.ok) {
            sendError(res, 400, "Please correct the highlighted fields.", r.errors);
            return;
        }
        sendJson(res, 200, {{"user", userJson(r.user)}});
    });

    server.Put("/api/users/me/password", [auth, users](const httplib::Request& req, httplib::Response& res) {
        auto u = requireUser(*auth, req, res);
        if (!u) return;
        json body;
        if (!parseBody(req, res, body)) return;
        auto r = users->changePassword(u->id, str(body, "currentPassword"), str(body, "newPassword"));
        if (!r.ok) {
            sendError(res, 400, "Password was not changed.", r.errors);
            return;
        }
        auth->revokeOtherSessions(u->id, bearerToken(req));
        sendJson(res, 200, {{"message", "Password changed."}});
    });
}

}  // namespace rtrs
