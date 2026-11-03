#pragma once
#include <string>

namespace rtrs {

// RTS-SR-003: TLS is enabled when both a certificate and private key are
// configured. When neither is set the service runs over HTTP, which is
// suitable only behind a TLS-terminating reverse proxy or for local use.
struct TlsConfig {
    std::string certPath, keyPath;

    bool enabled() const { return !certPath.empty() || !keyPath.empty(); }

    // Returns false and fills error when only one path is supplied or a
    // configured file cannot be read.
    bool valid(std::string& error) const;
};

TlsConfig tlsConfigFromEnvironment();

}  // namespace rtrs
