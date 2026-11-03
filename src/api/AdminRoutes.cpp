#include "api/AdminRoutes.h"

#include <algorithm>
#include <cctype>
#include <nlohmann/json.hpp>

#include "api/Authorization.h"
#include "services/AdminTrainService.h"

namespace rtrs {

namespace {
using nlohmann::json;

void reply(httplib::Response& res, int status, const json& body) {
    res.status = status;
    res.set_content(body.dump(), "application/json");
}

std::string text(const json& body, const char* key) {
    if (!body.is_object()) return "";
    auto it = body.find(key);
    return it != body.end() && it->is_string() ? it->get<std::string>() : "";
}

int integer(const json& body, const char* key) {
    if (!body.is_object()) return -1;
    auto it = body.find(key);
    if (it == body.end() || !it->is_number_integer() || *it < 0 || *it > 1000000) return -1;
    return it->get<int>();
}

TrainInput parseTrain(const json& body) {
    TrainInput input;
    input.number = text(body, "number");
    input.name = text(body, "name");
    auto stops = body.find("stops");
    if (stops != body.end() && stops->is_array()) {
        for (const auto& row : *stops) {
            Stop stop;
            stop.station = text(row, "station");
            stop.arrival = text(row, "arrival");
            stop.departure = text(row, "departure");
            stop.dayOffset = integer(row, "dayOffset");
            input.stops.push_back(stop);
        }
    }
    auto classes = body.find("classes");
    if (classes != body.end() && classes->is_array()) {
        for (const auto& row : *classes) {
            TrainClass cls;
            cls.code = text(row, "code");
            cls.name = text(row, "name");
            cls.fare = integer(row, "fare");
            cls.totalSeats = integer(row, "totalSeats");
            input.classes.push_back(cls);
        }
    }
    return input;
}

json trainJson(const ManagedTrain& train) {
    json stops = json::array(), classes = json::array();
    for (const auto& stop : train.details.stops)
        stops.push_back({{"station", stop.station}, {"arrival", stop.arrival},
                         {"departure", stop.departure}, {"dayOffset", stop.dayOffset}});
    for (const auto& cls : train.details.classes)
        classes.push_back({{"code", cls.code}, {"name", cls.name}, {"fare", cls.fare}, {"totalSeats", cls.totalSeats}});
    return {{"id", train.id}, {"active", train.active}, {"number", train.details.number}, {"name", train.details.name},
            {"stops", stops}, {"classes", classes}};
}

void writeReply(httplib::Response& res, const TrainWriteResult& result) {
    if (result.status < 300) reply(res, result.status, {{"train", trainJson(result.train)}});
    else reply(res, result.status, {{"error", result.status == 404 ? "Train not found." : "Could not save the train."},
                                    {"fields", result.errors}});
}

json auditJson(const AuditEvent& event) {
    return {{"id", event.id}, {"event", event.eventType}, {"userId", event.userId},
            {"bookingId", event.bookingId}, {"trainId", event.trainId}, {"pnr", event.pnr},
            {"refundAmount", event.refundAmount}, {"createdAt", event.createdAt}};
}

}

void registerAdminRoutes(httplib::Server& server, Database& db, std::shared_ptr<AuthService> auth,
                         std::shared_ptr<TrainService> trains, std::shared_ptr<AuditService> audit) {
    auto adminTrains = std::make_shared<AdminTrainService>(db, *trains);
    server.Get("/api/admin/me", [auth](const httplib::Request& req, httplib::Response& res) {
        auto user = requireAdmin(*auth, req, res);
        if (!user) return;
        res.set_content(nlohmann::json{{"user", {{"id", user->id}, {"name", user->name},
                          {"email", user->email}, {"role", user->role}}}}.dump(), "application/json");
    });
    server.Get("/api/admin/trains", [auth, trains, adminTrains](const httplib::Request& req, httplib::Response& res) {
        if (!requireAdmin(*auth, req, res)) return;
        json list = json::array();
        for (const auto& train : adminTrains->list()) list.push_back(trainJson(train));
        reply(res, 200, {{"trains", list}});
    });
    server.Get(R"(/api/admin/trains/([0-9]{1,18}))", [auth, trains, adminTrains](const httplib::Request& req, httplib::Response& res) {
        if (!requireAdmin(*auth, req, res)) return;
        auto train = adminTrains->findById(std::stoll(req.matches[1].str()));
        if (!train) reply(res, 404, {{"error", "Train not found."}});
        else reply(res, 200, {{"train", trainJson(*train)}});
    });
    server.Post("/api/admin/trains", [auth, trains, adminTrains](const httplib::Request& req, httplib::Response& res) {
        if (!requireAdmin(*auth, req, res)) return;
        auto body = json::parse(req.body, nullptr, false);
        if (!body.is_object()) {
            reply(res, 400, {{"error", "Request body must be a JSON object."}});
            return;
        }
        writeReply(res, adminTrains->create(parseTrain(body)));
    });
    server.Put(R"(/api/admin/trains/([0-9]{1,18}))", [auth, trains, adminTrains](const httplib::Request& req, httplib::Response& res) {
        if (!requireAdmin(*auth, req, res)) return;
        auto body = json::parse(req.body, nullptr, false);
        if (!body.is_object()) {
            reply(res, 400, {{"error", "Request body must be a JSON object."}});
            return;
        }
        writeReply(res, adminTrains->update(std::stoll(req.matches[1].str()), parseTrain(body)));
    });
    server.Put(R"(/api/admin/trains/([0-9]{1,18})/deactivate)", [auth, trains, adminTrains](const httplib::Request& req, httplib::Response& res) {
        if (!requireAdmin(*auth, req, res)) return;
        writeReply(res, adminTrains->deactivate(std::stoll(req.matches[1].str())));
    });
    // RTS-F-018 / RTS-NF-008: administrators can retrieve the audit trail.
    server.Get("/api/admin/audit", [auth, audit](const httplib::Request& req, httplib::Response& res) {
        if (!requireAdmin(*auth, req, res)) return;
        int limit = 100;
        const std::string raw = req.get_param_value("limit");
        if (!raw.empty()) {
            bool digits = raw.size() <= 3 &&
                          std::all_of(raw.begin(), raw.end(), [](unsigned char c) { return std::isdigit(c) != 0; });
            int parsed = digits ? std::stoi(raw) : 0;
            if (parsed < 1 || parsed > 500) {
                reply(res, 400, {{"error", "Limit must be between 1 and 500."}});
                return;
            }
            limit = parsed;
        }
        json events = json::array();
        for (const auto& event : audit->list(limit)) events.push_back(auditJson(event));
        reply(res, 200, {{"events", events}});
    });
}

}
