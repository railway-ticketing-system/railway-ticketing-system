#pragma once
#include <string>
#include <vector>

#include "db/Database.h"

namespace rtrs {

struct AuditEvent {
    long long id = 0, userId = 0, bookingId = 0, trainId = 0;
    int refundAmount = 0;
    std::string eventType, pnr, createdAt;
};

// RTS-F-018 / RTS-NF-008: persistent, timestamped booking and cancellation
// events. Records contain identifiers only, never credentials or passenger
// personal data, so they remain safe to inspect during an audit.
class AuditService {
public:
    explicit AuditService(Database& db) : db_(db) {}

    void recordBookingCreated(long long userId, long long bookingId, long long trainId, const std::string& pnr);
    void recordBookingCancelled(long long userId, long long bookingId, long long trainId, const std::string& pnr,
                                int refundAmount);

    // Newest first. Limit is clamped to 1..500 by the caller; the service keeps
    // the bound as a defensive measure.
    std::vector<AuditEvent> list(int limit) const;

private:
    void record(const std::string& eventType, long long userId, long long bookingId, long long trainId,
                const std::string& pnr, int refundAmount);

    Database& db_;
};

}  // namespace rtrs
