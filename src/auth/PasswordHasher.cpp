#include "auth/PasswordHasher.h"

#include <openssl/crypto.h>
#include <openssl/evp.h>
#include <openssl/rand.h>

#include <stdexcept>
#include <vector>

namespace rtrs {

namespace {
constexpr size_t kSaltLen = 16;
constexpr size_t kHashLen = 32;

bool fromHex(const std::string& hex, std::vector<unsigned char>& out) {
    if (hex.empty() || hex.size() % 2) return false;
    out.clear();
    auto val = [](char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        return -1;
    };
    for (size_t i = 0; i < hex.size(); i += 2) {
        int hi = val(hex[i]), lo = val(hex[i + 1]);
        if (hi < 0 || lo < 0) return false;
        out.push_back(static_cast<unsigned char>(hi * 16 + lo));
    }
    return true;
}

std::vector<unsigned char> derive(const std::string& pw, const std::vector<unsigned char>& salt, int iters) {
    std::vector<unsigned char> out(kHashLen);
    if (PKCS5_PBKDF2_HMAC(pw.data(), static_cast<int>(pw.size()), salt.data(), static_cast<int>(salt.size()),
                          iters, EVP_sha256(), static_cast<int>(out.size()), out.data()) != 1)
        throw std::runtime_error("password hashing failed");
    return out;
}
}  // namespace

std::string toHex(const unsigned char* data, size_t len) {
    static const char* d = "0123456789abcdef";
    std::string s;
    s.reserve(len * 2);
    for (size_t i = 0; i < len; ++i) {
        s.push_back(d[data[i] >> 4]);
        s.push_back(d[data[i] & 0xF]);
    }
    return s;
}

std::string randomHex(size_t bytes) {
    std::vector<unsigned char> buf(bytes);
    if (RAND_bytes(buf.data(), static_cast<int>(bytes)) != 1) throw std::runtime_error("random generation failed");
    return toHex(buf.data(), buf.size());
}

std::string PasswordHasher::hash(const std::string& password) const {
    std::vector<unsigned char> salt(kSaltLen);
    if (RAND_bytes(salt.data(), static_cast<int>(salt.size())) != 1) throw std::runtime_error("random generation failed");
    auto h = derive(password, salt, iterations_);
    return "pbkdf2$" + std::to_string(iterations_) + "$" + toHex(salt.data(), salt.size()) + "$" +
           toHex(h.data(), h.size());
}

bool PasswordHasher::verify(const std::string& password, const std::string& stored) const {
    // split on '$' -> scheme, iterations, salt, hash
    std::vector<std::string> parts;
    size_t start = 0, pos;
    while ((pos = stored.find('$', start)) != std::string::npos) {
        parts.push_back(stored.substr(start, pos - start));
        start = pos + 1;
    }
    parts.push_back(stored.substr(start));
    if (parts.size() != 4 || parts[0] != "pbkdf2") return false;

    int iters = 0;
    try {
        iters = std::stoi(parts[1]);
    } catch (...) {
        return false;
    }
    if (iters <= 0) return false;
    std::vector<unsigned char> salt, expected;
    if (!fromHex(parts[2], salt) || !fromHex(parts[3], expected) || expected.size() != kHashLen) return false;

    auto actual = derive(password, salt, iters);
    return CRYPTO_memcmp(actual.data(), expected.data(), kHashLen) == 0;
}

}  // namespace rtrs
