#pragma once
#include <optional>
#include <string>

#include "auth/PasswordHasher.h"
#include "db/Database.h"
#include "util/Validation.h"

namespace rtrs {

struct User {
    long long id = 0;
    std::string name, email, phone, role, createdAt;
};

struct RegisterResult {
    enum class Status { Created, ValidationFailed, EmailTaken } status = Status::ValidationFailed;
    User user;
    FieldErrors errors;
};

struct ProfileResult {
    bool ok = false;
    User user;
    FieldErrors errors;
};

class UserService {
public:
    UserService(Database& db, PasswordHasher hasher) : db_(db), hasher_(hasher) {}

    // RTS-F-001: create a passenger account. Email is stored lower-case and must be unique.
    RegisterResult registerUser(RegistrationInput input);

    // RTS-F-001 (profile): name and phone may change; email is the login identity and is immutable.
    ProfileResult updateProfile(long long userId, ProfileUpdate input);
    // Verifies the current password, then stores a new hash. Errors are keyed by field name.
    ProfileResult changePassword(long long userId, const std::string& current, const std::string& next);

    std::optional<User> findById(long long id);
    // Returns the user together with the stored password hash, or nullopt if the email is unknown.
    std::optional<std::pair<User, std::string>> findCredentials(const std::string& email);

private:
    bool emailExists(const std::string& email);

    Database& db_;
    PasswordHasher hasher_;
};

}  // namespace rtrs
