#include "api/Routes.h"

#include <nlohmann/json.hpp>
#include <memory>

#include "db/Schema.h"
#include "db/Seed.h"
#include "services/AuthService.h"
#include "services/BookingService.h"
#include "services/TrainService.h"
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

json trainSummaryJson(const TrainSummary& t) {
    return {{"id", t.id},       {"number", t.number}, {"name", t.name},
            {"source", t.source}, {"destination", t.destination},
            {"departure", t.departure}, {"arrival", t.arrival},
            {"durationMinutes", t.durationMinutes}, {"fromFare", t.fromFare}};
}

long long intField(const json& body, const char* key) {
    auto it = body.find(key);
    if (it == body.end()) return 0;
    if (it->is_number_integer()) return it->get<long long>();
    if (it->is_string()) {
        try {
            return std::stoll(it->get<std::string>());
        } catch (...) {
            return 0;
        }
    }
    return 0;
}

std::vector<PassengerInput> parsePassengers(const json& body) {
    std::vector<PassengerInput> passengers;
    auto it = body.find("passengers");
    if (it == body.end() || !it->is_array()) return passengers;
    for (const auto& item : *it) {
        PassengerInput passenger;
        if (item.is_object()) {
            passenger.name = str(item, "name");
            passenger.gender = str(item, "gender");
            auto age = item.find("age");
            if (age != item.end()) {
                if (age->is_number_integer()) passenger.age = std::to_string(age->get<long long>());
                else if (age->is_string()) passenger.age = age->get<std::string>();
            }
        }
        passengers.push_back(std::move(passenger));
    }
    return passengers;
}

BookingInput parseBooking(const json& body) {
    BookingInput input;
    input.trainId = intField(body, "trainId");
    input.travelDate = str(body, "date");
    input.classCode = str(body, "class");
    input.passengers = parsePassengers(body);
    return input;
}

json bookingPreviewJson(const BookingPreview& p) {
    return {{"trainId", p.trainId},         {"trainNumber", p.trainNumber},
            {"trainName", p.trainName},     {"date", p.travelDate},
            {"class", p.classCode},         {"className", p.className},
            {"farePerPassenger", p.farePerPassenger},
            {"passengerCount", p.passengerCount},
            {"totalFare", p.totalFare},     {"availableSeats", p.availableSeats}};
}

json bookingJson(const Booking& b) {
    json passengers = json::array();
    for (const auto& p : b.passengers) {
        passengers.push_back({{"name", p.name},
                              {"age", p.age},
                              {"gender", p.gender},
                              {"seatNumber", p.seatNumber}});
    }
    return {{"id", b.id},
            {"pnr", b.pnr},
            {"status", b.status},
            {"trainId", b.trainId},
            {"trainNumber", b.trainNumber},
            {"trainName", b.trainName},
            {"date", b.travelDate},
            {"class", b.classCode},
            {"className", b.className},
            {"farePerPassenger", b.farePerPassenger},
            {"passengerCount", b.passengerCount},
            {"totalFare", b.totalFare},
            {"createdAt", b.createdAt},
            {"cancelledAt", b.cancelledAt},
            {"passengers", passengers}};
}

}  // namespace

void registerRoutes(httplib::Server& server, Database& db, int pbkdf2Iterations,
                    CancellationPolicy policy) {
    initSchema(db);
    seedDemoData(db);
    auto users = std::make_shared<UserService>(db, PasswordHasher(pbkdf2Iterations));
    auto auth = std::make_shared<AuthService>(db, *users, PasswordHasher(pbkdf2Iterations));
    auto trains = std::make_shared<TrainService>(db);
    auto bookings = std::make_shared<BookingService>(db, *trains, policy);

    server.set_payload_max_length(64 * 1024);
    // RTS-NF-003: never leak internals in error responses.
    server.set_exception_handler([](const httplib::Request&, httplib::Response& res, std::exception_ptr) {
        sendError(res, 500, "Something went wrong. Please try again.");
    });

    server.Get("/api/health", [](const httplib::Request&, httplib::Response& res) {
        sendJson(res, 200, {{"status", "ok"}});
    });

    // RTRS-13 / RTS-F-003
    server.Get("/api/trains/search", [trains](const httplib::Request& req, httplib::Response& res) {
        SearchQuery query;
        query.source = req.get_param_value("source");
        query.destination = req.get_param_value("destination");
        query.date = req.get_param_value("date");
        query.travelClass = req.get_param_value("class");
        query.maxFare = req.get_param_value("maxFare");
        query.departureFrom = req.get_param_value("departureFrom");
        query.departureTo = req.get_param_value("departureTo");
        query.sort = req.get_param_value("sort");
        query.order = req.get_param_value("order");
        auto result = trains->search(query);
        if (!result.ok()) {
            sendError(res, 400, "Please correct the highlighted fields.", result.errors);
            return;
        }
        json list = json::array();
        for (const auto& train : result.trains) list.push_back(trainSummaryJson(train));
        sendJson(res, 200, {{"date", trim(query.date)}, {"count", list.size()}, {"trains", list}});
    });

    // RTRS-14 / RTS-F-004
    server.Get(R"(/api/trains/(\d+)/schedule)", [trains](const httplib::Request& req, httplib::Response& res) {
        auto schedule = trains->findById(std::stoll(req.matches[1].str()));
        if (!schedule) {
            sendError(res, 404, "Train not found.");
            return;
        }
        json stops = json::array();
        for (const auto& stop : schedule->stops) {
            stops.push_back({{"station", stop.station}, {"arrival", stop.arrival},
                             {"departure", stop.departure}, {"dayOffset", stop.dayOffset}});
        }
        sendJson(res, 200,
                 {{"train", {{"id", schedule->id}, {"number", schedule->number}, {"name", schedule->name}}},
                  {"stops", stops}});
    });

    // RTRS-15 / RTS-F-005
    server.Get(R"(/api/trains/(\d+)/availability)", [trains](const httplib::Request& req, httplib::Response& res) {
        long long id = std::stoll(req.matches[1].str());
        if (!trains->findById(id)) {
            sendError(res, 404, "Train not found.");
            return;
        }
        std::string date = trim(req.get_param_value("date"));
        if (!isValidDate(date)) {
            sendError(res, 400, "Enter a valid travel date (YYYY-MM-DD).",
                      {{"date", "Enter a valid travel date (YYYY-MM-DD)."}});
            return;
        }
        json list = json::array();
        for (const auto& entry : trains->availability(id, date)) {
            list.push_back({{"code", entry.code},
                            {"name", entry.name},
                            {"totalSeats", entry.totalSeats},
                            {"bookedSeats", entry.bookedSeats},
                            {"availableSeats", entry.availableSeats},
                            {"fare", entry.fare}});
        }
        sendJson(res, 200, {{"date", date}, {"classes", list}});
    });

    // RTRS-16 / RTS-F-006
    server.Get(R"(/api/trains/(\d+)/fares)", [trains](const httplib::Request& req, httplib::Response& res) {
        long long id = std::stoll(req.matches[1].str());
        if (!trains->findById(id)) {
            sendError(res, 404, "Train not found.");
            return;
        }
        json list = json::array();
        for (const auto& cls : trains->fares(id)) {
            list.push_back({{"code", cls.code}, {"name", cls.name}, {"fare", cls.fare}});
        }
        sendJson(res, 200, {{"classes", list}});
    });

    // RTRS-18 / RTS-F-007
    server.Post("/api/bookings/preview", [auth, bookings](const httplib::Request& req, httplib::Response& res) {
        auto user = requireUser(*auth, req, res);
        if (!user) return;
        json body;
        if (!parseBody(req, res, body)) return;
        auto input = parseBooking(body);
        input.userId = user->id;
        auto result = bookings->preview(input);
        if (!result.ok) {
            bool soldOut = result.conflict == "sold_out";
            sendError(res, soldOut ? 409 : 400,
                      soldOut ? "Not enough seats are available." : "Please correct the highlighted fields.",
                      result.errors);
            return;
        }
        sendJson(res, 200, {{"preview", bookingPreviewJson(result.preview)}});
    });

    // RTRS-21 / RTS-F-007
    server.Post("/api/bookings", [auth, bookings](const httplib::Request& req, httplib::Response& res) {
        auto user = requireUser(*auth, req, res);
        if (!user) return;
        json body;
        if (!parseBody(req, res, body)) return;
        auto input = parseBooking(body);
        input.userId = user->id;
        auto result = bookings->confirm(input);
        if (!result.ok) {
            if (result.conflict == "sold_out") {
                sendError(res, 409, "Not enough seats are available.", result.errors);
            } else if (result.conflict == "pnr_unavailable") {
                sendError(res, 409, "Could not generate a PNR. Please try again.", result.errors);
            } else {
                sendError(res, 400, "Please correct the highlighted fields.", result.errors);
            }
            return;
        }
        sendJson(res, 201, {{"booking", bookingJson(result.booking)}});
    });

    // RTRS-24 / RTS-F-008
    server.Get("/api/bookings", [auth, bookings](const httplib::Request& req, httplib::Response& res) {
        auto user = requireUser(*auth, req, res);
        if (!user) return;
        json list = json::array();
        for (const auto& booking : bookings->listForUser(user->id)) list.push_back(bookingJson(booking));
        sendJson(res, 200, {{"bookings", list}});
    });

    // RTRS-23 / RTS-F-008
    server.Get(R"(/api/bookings/([0-9]+))", [auth, bookings](const httplib::Request& req, httplib::Response& res) {
        auto user = requireUser(*auth, req, res);
        if (!user) return;
        auto booking = bookings->findByPnr(req.matches[1]);
        if (!booking || booking->userId != user->id) {
            sendError(res, 404, "Booking not found.");
            return;
        }
        sendJson(res, 200, {{"booking", bookingJson(*booking)}});
    });

    // RTRS-25 / RTS-F-013
    server.Post(R"(/api/bookings/([0-9]+)/cancel)", [auth, bookings](const httplib::Request& req, httplib::Response& res) {
        auto user = requireUser(*auth, req, res);
        if (!user) return;
        auto result = bookings->cancel(std::stoll(req.matches[1].str()));
        if (!result.ok) {
            if (result.conflict == "not_found") {
                sendError(res, 404, "Booking not found.");
            } else {
                sendError(res, 409, result.errors["booking"], result.errors);
            }
            return;
        }
        sendJson(res, 200, {{"booking", bookingJson(result.booking)}});
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
