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

bool isValidStation(const std::string& station) {
    if (station.size() < 2 || station.size() > 60) return false;
    bool hasLetter = false;
    for (unsigned char c : station) {
        if (std::isalpha(c)) hasLetter = true;
        else if (c != ' ' && c != '.' && c != '\'' && c != '-' && c != '&' && c != '(' && c != ')')
            return false;
    }
    return hasLetter;
}

bool isValidDate(const std::string& date) {
    if (date.size() != 10 || date[4] != '-' || date[7] != '-') return false;
    for (size_t i : {0, 1, 2, 3, 5, 6, 8, 9})
        if (!std::isdigit(static_cast<unsigned char>(date[i]))) return false;

    int year = std::stoi(date.substr(0, 4));
    int month = std::stoi(date.substr(5, 2));
    int day = std::stoi(date.substr(8, 2));
    if (year < 1900 || year > 2200 || month < 1 || month > 12 || day < 1) return false;

    static const int daysInMonth[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int limit = daysInMonth[month - 1];
    bool leap = (year % 4 == 0 && year % 100 != 0) || year % 400 == 0;
    if (month == 2 && leap) limit = 29;
    return day <= limit;
}

bool isValidTime(const std::string& time) {
    if (time.size() != 5 || time[2] != ':') return false;
    for (size_t i : {0, 1, 3, 4})
        if (!std::isdigit(static_cast<unsigned char>(time[i]))) return false;
    int hour = std::stoi(time.substr(0, 2));
    int minute = std::stoi(time.substr(3, 2));
    return hour <= 23 && minute <= 59;
}

bool isValidClassCode(const std::string& code) {
    return code == "SL" || code == "3A" || code == "2A" || code == "1A" || code == "CC" || code == "EC";
}

bool isValidSortField(const std::string& field) {
    return field == "departure" || field == "price" || field == "duration";
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
