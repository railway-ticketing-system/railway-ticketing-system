#pragma once
#include <string>

namespace rtrs {

// PBKDF2-HMAC-SHA256 with a random per-password salt (RTS-NF-005 / RTS-SR-001).
// Stored format: pbkdf2$<iterations>$<salt hex>$<hash hex>
class PasswordHasher {
public:
    static constexpr int kDefaultIterations = 600000;
    static constexpr int kMinIterations = 100000;
    static constexpr int kMaxIterations = 10000000;
    explicit PasswordHasher(int iterations = kDefaultIterations) : iterations_(iterations) {}

    std::string hash(const std::string& password) const;
    // Constant-time comparison; returns false for malformed or implausibly
    // expensive stored values.
    bool verify(const std::string& password, const std::string& stored) const;

    // Iteration count encoded in a stored hash, or 0 if it is malformed.
    int storedIterations(const std::string& stored) const;
    // True when a successfully verified hash should be replaced: either it is
    // weaker than this hasher or it is below the accepted security floor.
    bool needsRehash(const std::string& stored) const;

private:
    int iterations_;
};

std::string toHex(const unsigned char* data, size_t len);
std::string randomHex(size_t bytes);
std::string sha256Hex(const std::string& data);

}  // namespace rtrs
