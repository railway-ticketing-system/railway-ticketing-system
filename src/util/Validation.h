#pragma once
#include <map>
#include <string>

namespace rtrs {

// field name -> human readable message. Empty map means the input is valid.
using FieldErrors = std::map<std::string, std::string>;

std::string trim(const std::string& s);

bool isValidName(const std::string& name);      // 2-60 chars: letters, spaces, . ' -
bool isValidEmail(const std::string& email);    // simple RFC-5322 subset, max 254 chars
bool isValidPhone(const std::string& phone);    // 10 digits starting with 6-9
bool isValidPassword(const std::string& pw);    // 8-128 chars, >=1 letter and >=1 digit

struct RegistrationInput {
    std::string name, email, phone, password;
};

FieldErrors validateRegistration(const RegistrationInput& in);

struct ProfileUpdate {
    std::string name, phone;
};

FieldErrors validateProfileUpdate(const ProfileUpdate& in);

}  // namespace rtrs
