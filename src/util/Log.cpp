#include "util/Log.h"

#include <algorithm>
#include <ctime>
#include <iostream>
#include <regex>

namespace rtrs {

namespace {

bool isControl(unsigned char c) { return c < 0x20 || c == 0x7f; }

}  // namespace

std::string redactForLog(const std::string& text) {
    static const std::regex bearer(R"((Bearer)\s+[A-Za-z0-9._~+/=-]+)", std::regex_constants::icase);
    static const std::regex secret(
        R"((password|passwd|token|secret|authorization|api[_-]?key)(\s*[:=]\s*)([^\s,&;]+))",
        std::regex_constants::icase);
    return std::regex_replace(std::regex_replace(text, bearer, "$1 [REDACTED]"), secret, "$1$2[REDACTED]");
}

std::string safeRequestTarget(const std::string& target) {
    std::string path = target.substr(0, target.find_first_of("?#"));
    path.erase(std::remove_if(path.begin(), path.end(), [](unsigned char c) { return isControl(c); }), path.end());
    constexpr size_t kMaxLength = 200;
    if (path.size() > kMaxLength) path.resize(kMaxLength);
    return path;
}

void logAccess(const std::string& method, const std::string& target, int status, const std::string& remote) {
    char timestamp[21];
    std::time_t now = std::time(nullptr);
    std::tm utc{};
    gmtime_r(&now, &utc);
    std::strftime(timestamp, sizeof timestamp, "%Y-%m-%dT%H:%M:%SZ", &utc);

    std::cerr << redactForLog(std::string(timestamp) + " " + method + " " + safeRequestTarget(target) + " " +
                              std::to_string(status) + " " + remote)
              << std::endl;
}

}  // namespace rtrs
