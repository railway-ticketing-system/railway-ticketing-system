#include "services/UserService.h"

#include <algorithm>
#include <cctype>

namespace rtrs {

namespace {
User userFromRow(const Database::Row& r) { return User{std::stoll(r[0]), r[1], r[2], r[3], r[4], r[5]}; }
const char* kUserCols = "id, name, email, phone, role, created_at";
}  // namespace

bool UserService::emailExists(const std::string& email) {
    return !db_.query("SELECT 1 FROM users WHERE email = ?", {email}).empty();
}

RegisterResult UserService::registerUser(RegistrationInput in) {
    RegisterResult result;
    in.name = trim(in.name);
    in.email = trim(in.email);
    in.phone = trim(in.phone);
    std::transform(in.email.begin(), in.email.end(), in.email.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });

    result.errors = validateRegistration(in);
    if (!result.errors.empty()) return result;

    if (emailExists(in.email)) {
        result.status = RegisterResult::Status::EmailTaken;
        result.errors["email"] = "An account with this email already exists.";
        return result;
    }

    try {
        db_.execute("INSERT INTO users (name, email, phone, password_hash) VALUES (?, ?, ?, ?)",
                    {in.name, in.email, in.phone, hasher_.hash(in.password)});
    } catch (const std::exception&) {
        // lost a race against a concurrent registration of the same email
        if (!emailExists(in.email)) throw;
        result.status = RegisterResult::Status::EmailTaken;
        result.errors["email"] = "An account with this email already exists.";
        return result;
    }

    auto rows = db_.query(std::string("SELECT ") + kUserCols + " FROM users WHERE email = ?", {in.email});
    result.user = userFromRow(rows.at(0));
    result.status = RegisterResult::Status::Created;
    result.errors.clear();
    return result;
}

std::optional<User> UserService::findById(long long id) {
    auto rows = db_.query(std::string("SELECT ") + kUserCols + " FROM users WHERE id = ?", {std::to_string(id)});
    if (rows.empty()) return std::nullopt;
    return userFromRow(rows[0]);
}

std::optional<std::pair<User, std::string>> UserService::findCredentials(const std::string& email) {
    auto rows = db_.query(std::string("SELECT ") + kUserCols + ", password_hash FROM users WHERE email = ?", {email});
    if (rows.empty()) return std::nullopt;
    return std::make_pair(userFromRow(rows[0]), rows[0][6]);
}

}  // namespace rtrs
