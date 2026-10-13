#include "services/UserService.h"

#include <algorithm>
#include <cctype>

namespace rtrs {

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

    auto rows = db_.query("SELECT id, name, email, phone, role, created_at FROM users WHERE email = ?", {in.email});
    const auto& r = rows.at(0);
    result.user = User{std::stoll(r[0]), r[1], r[2], r[3], r[4], r[5]};
    result.status = RegisterResult::Status::Created;
    result.errors.clear();
    return result;
}

}  // namespace rtrs
