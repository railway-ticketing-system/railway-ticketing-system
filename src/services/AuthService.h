#pragma once
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <string>

#include "services/UserService.h"

namespace rtrs {

struct LoginResult {
    enum class Status { Ok, InvalidCredentials, Locked } status = Status::InvalidCredentials;
    std::string token;        // raw bearer token, returned to the client exactly once
    long long expiresAt = 0;  // unix seconds
    User user;
};

class AuthService {
public:
    using Clock = std::function<long long()>;  // unix seconds; injectable for tests
    static constexpr long long kSessionTtl = 24 * 3600;
    static constexpr int kMaxFailures = 5;
    static constexpr long long kLockSeconds = 300;

    AuthService(Database& db, UserService& users, PasswordHasher hasher, Clock clock = nullptr);

    // RTS-F-002: authenticate a registered passenger and open a session.
    LoginResult login(const std::string& email, const std::string& password);

    // Resolves a raw bearer token to its user if the session exists and has not expired.
    std::optional<User> authenticate(const std::string& token);

    // RTS-F-002 (session end): invalidates the session immediately. Returns false if it did not exist.
    bool logout(const std::string& token);

private:
    struct Attempts {
        int failures = 0;
        long long lockedUntil = 0;
    };

    Database& db_;
    UserService& users_;
    PasswordHasher hasher_;
    Clock clock_;
    std::string dummyHash_;  // verified against when the email is unknown, to keep timing similar
    std::mutex mutex_;
    std::map<std::string, Attempts> attempts_;
};

}  // namespace rtrs
