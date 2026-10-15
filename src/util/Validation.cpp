#include "util/Validation.h"

#include <cctype>
#include <regex>

namespace rtrs {

std::string trim(const std::string& s) {
    size_t b = 0, e = s.size();
    while (b < e && std::isspace(static_cast<unsigned char>(s[b]))) ++b;
    while (e > b && std::isspace(static_cast<unsigned char>(s[e - 1]))) --e;
    return s.substr(b, e - b);
}

bool isValidName(const std::string& name) {
    if (name.size() < 2 || name.size() > 60) return false;
    bool hasLetter = false;
    for (unsigned char c : name) {
        if (std::isalpha(c)) hasLetter = true;
        else if (c != ' ' && c != '.' && c != '\'' && c != '-') return false;
    }
    return hasLetter;
}

bool isValidEmail(const std::string& email) {
    if (email.size() > 254) return false;
    static const std::regex re(R"(^[A-Za-z0-9._%+-]{1,64}@[A-Za-z0-9-]+(\.[A-Za-z0-9-]+)+$)");
    return std::regex_match(email, re);
}

bool isValidPhone(const std::string& phone) {
    if (phone.size() != 10 || phone[0] < '6' || phone[0] > '9') return false;
    for (unsigned char c : phone)
        if (!std::isdigit(c)) return false;
    return true;
}

bool isValidPassword(const std::string& pw) {
    if (pw.size() < 8 || pw.size() > 128) return false;
    bool letter = false, digit = false;
    for (unsigned char c : pw) {
        if (std::isalpha(c)) letter = true;
        if (std::isdigit(c)) digit = true;
    }
    return letter && digit;
}

FieldErrors validateRegistration(const RegistrationInput& in) {
    FieldErrors errs;
    if (!isValidName(in.name)) errs["name"] = "Name must be 2-60 characters (letters, spaces, . ' -).";
    if (!isValidEmail(in.email)) errs["email"] = "Enter a valid email address.";
    if (!isValidPhone(in.phone)) errs["phone"] = "Phone must be 10 digits starting with 6-9.";
    if (!isValidPassword(in.password))
        errs["password"] = "Password must be 8-128 characters with at least one letter and one digit.";
    return errs;
}

FieldErrors validateProfileUpdate(const ProfileUpdate& in) {
    FieldErrors errs;
    if (!isValidName(in.name)) errs["name"] = "Name must be 2-60 characters (letters, spaces, . ' -).";
    if (!isValidPhone(in.phone)) errs["phone"] = "Phone must be 10 digits starting with 6-9.";
    return errs;
}

}  // namespace rtrs
