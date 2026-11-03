#include "services/AuditService.h"

#include <algorithm>

namespace rtrs {

void AuditService::record(const std::string& eventType, long long userId, long long bookingId, long long trainId,
                          const std::string& pnr, int refundAmount) {
    db_.execute(
        "INSERT INTO audit_events (event_type, user_id, booking_id, train_id, pnr, refund_amount)"
        " VALUES (?, ?, ?, ?, ?, ?)",
        {eventType, std::to_string(userId), std::to_string(bookingId), std::to_string(trainId), pnr,
         std::to_string(refundAmount)});
}

void AuditService::recordBookingCreated(long long userId, long long bookingId, long long trainId,
                                        const std::string& pnr) {
    record("BOOKING_CREATED", userId, bookingId, trainId, pnr, 0);
}

void AuditService::recordBookingCancelled(long long userId, long long bookingId, long long trainId,
                                          const std::string& pnr, int refundAmount) {
    record("BOOKING_CANCELLED", userId, bookingId, trainId, pnr, refundAmount);
}

std::vector<AuditEvent> AuditService::list(int limit) const {
    limit = std::max(1, std::min(limit, 500));
    std::vector<AuditEvent> events;
    for (const auto& row : db_.query(
             "SELECT id, event_type, user_id, booking_id, train_id, pnr, refund_amount, created_at"
             " FROM audit_events ORDER BY id DESC LIMIT ?",
             {std::to_string(limit)})) {
        AuditEvent event;
        event.id = std::stoll(row[0]);
        event.eventType = row[1];
        event.userId = std::stoll(row[2]);
        event.bookingId = std::stoll(row[3]);
        event.trainId = std::stoll(row[4]);
        event.pnr = row[5];
        event.refundAmount = std::stoi(row[6]);
        event.createdAt = row[7];
        events.push_back(std::move(event));
    }
    return events;
}

}  // namespace rtrs
