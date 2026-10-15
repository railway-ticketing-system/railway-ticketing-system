#include "services/AuthService.h"

#include <algorithm>
#include <cctype>
#include <ctime>

namespace rtrs {

AuthService::AuthService(Database& db, UserService& users, PasswordHasher hasher, Clock clock)
    : db_(db), users_(users), hasher_(hasher), clock_(std::move(clock)), dummyHash_(hasher.hash("dummy-password-1")) {
    if (!clock_) clock_ = [] { return static_cast<long long>(std::time(nullptr)); };
}

LoginResult AuthService::login(const std::string& rawEmail, const std::string& password) {
    LoginResult result;
    std::string email = trim(rawEmail);
    std::transform(email.begin(), email.end(), email.begin(), [](unsigned char c) { return std::tolower(c); });
    const long long now = clock_();

    {
        std::lock_guard<std::mutex> lock(mutex_);
        auto it = attempts_.find(email);
        if (it != attempts_.end() && it->second.lockedUntil > now) {
            result.status = LoginResult::Status::Locked;
            return result;
        }
    }

    auto creds = users_.findCredentials(email);
    // Always run one hash verification so unknown emails cost the same as wrong passwords.
    bool ok = creds ? hasher_.verify(password, creds->second) : (hasher_.verify(password, dummyHash_), false);

    std::lock_guard<std::mutex> lock(mutex_);
    if (!ok) {
        auto& a = attempts_[email];
        if (a.lockedUntil != 0 && a.lockedUntil <= now) a = Attempts{};  // lock expired
        if (++a.failures >= kMaxFailures) a.lockedUntil = now + kLockSeconds;
        return result;  // InvalidCredentials
    }
    attempts_.erase(email);

    result.token = randomHex(32);
    result.expiresAt = now + kSessionTtl;
    db_.execute("INSERT INTO sessions (token_hash, user_id, expires_at) VALUES (?, ?, ?)",
                {sha256Hex(result.token), std::to_string(creds->first.id), std::to_string(result.expiresAt)});
    result.user = creds->first;
    result.status = LoginResult::Status::Ok;
    return result;
}

std::optional<User> AuthService::authenticate(const std::string& token) {
    if (token.empty() || token.size() > 256) return std::nullopt;
    auto rows = db_.query("SELECT user_id, expires_at FROM sessions WHERE token_hash = ?", {sha256Hex(token)});
    if (rows.empty()) return std::nullopt;
    if (std::stoll(rows[0][1]) <= clock_()) {
        db_.execute("DELETE FROM sessions WHERE token_hash = ?", {sha256Hex(token)});
        return std::nullopt;
    }
    return users_.findById(std::stoll(rows[0][0]));
}

bool AuthService::logout(const std::string& token) {
    if (!authenticate(token)) return false;
    db_.execute("DELETE FROM sessions WHERE token_hash = ?", {sha256Hex(token)});
    return true;
}

}  // namespace rtrs
