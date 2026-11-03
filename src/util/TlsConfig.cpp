#include "util/TlsConfig.h"

#include <cstdlib>
#include <fstream>

namespace rtrs {

namespace {

bool readable(const std::string& path) { return !path.empty() && std::ifstream(path).good(); }

}  // namespace

bool TlsConfig::valid(std::string& error) const {
    if (certPath.empty() != keyPath.empty()) {
        error = "set both RTRS_TLS_CERT and RTRS_TLS_KEY, or neither";
        return false;
    }
    if (!certPath.empty() && !readable(certPath)) {
        error = "RTRS_TLS_CERT file is not readable";
        return false;
    }
    if (!keyPath.empty() && !readable(keyPath)) {
        error = "RTRS_TLS_KEY file is not readable";
        return false;
    }
    return true;
}

TlsConfig tlsConfigFromEnvironment() {
    TlsConfig config;
    if (const char* cert = std::getenv("RTRS_TLS_CERT")) config.certPath = cert;
    if (const char* key = std::getenv("RTRS_TLS_KEY")) config.keyPath = key;
    return config;
}

}  // namespace rtrs
