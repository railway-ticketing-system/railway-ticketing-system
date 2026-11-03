#pragma once
#include <string>

namespace rtrs {

// RTS-SR-005: logs must never contain passwords, session tokens or other
// authentication secrets. Values following common secret keys are redacted.
std::string redactForLog(const std::string& text);

// Access logs record only the path (never the query string), strip control
// characters, and cap the length so client input cannot forge log lines.
std::string safeRequestTarget(const std::string& target);

void logAccess(const std::string& method, const std::string& target, int status, const std::string& remote);

}  // namespace rtrs
