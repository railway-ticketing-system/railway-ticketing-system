# Code Coverage Report

Generated 2026-11-12T16:45:25.898236+00:00. Application source: `6b5464298c949806ebba0b7bd1d903c962b5255d`.

Environment: Linux-7.2.14-200.fc44.x86_64-x86_64-with-glibc2.43; g++ (GCC) 16.2.1 20261029 (Red Hat 16.2.1-3); gcovr 8.8.

**94.7%**: **1476 / 1558** measured lines covered.

## Scope and method

GCC/gcov instrumentation with gcovr, restricted to project `src/` files and headers; `src/main.cpp`, tests and downloaded dependencies are excluded. The executable startup, CLI and real TLS path are additionally exercised by the system validation runner but are not included in this coverage percentage. No exclusions for exception branches or unreachable branches are applied. Compiler-generated C++ exception/cleanup edges remain in the branch denominator. No minimum percentage was specified in the five course PDFs.

Negative-hit parse handling matches the existing CI workaround: `--gcov-ignore-parse-errors negative_hits.warn_once_per_file`. The collection log is included as `coverage-collection.log`; inspect warnings when regenerating.

[Interactive source coverage](coverage.html) · [Raw coverage JSON](coverage.json) · [Summary JSON](coverage-summary.json)

| File | Covered | Total | Percent |
|---|---:|---:|---:|
| `src/api/AdminRoutes.cpp` | 111 | 115 | 96.5 |
| `src/api/Authorization.h` | 20 | 20 | 100.0 |
| `src/api/Routes.cpp` | 318 | 330 | 96.4 |
| `src/auth/PasswordHasher.cpp` | 68 | 75 | 90.7 |
| `src/auth/PasswordHasher.h` | 1 | 1 | 100.0 |
| `src/db/Database.cpp` | 42 | 46 | 91.3 |
| `src/db/Schema.cpp` | 42 | 45 | 93.3 |
| `src/db/Seed.cpp` | 24 | 24 | 100.0 |
| `src/services/AdminTrainService.cpp` | 187 | 190 | 98.4 |
| `src/services/AdminTrainService.h` | 1 | 1 | 100.0 |
| `src/services/AuditService.cpp` | 29 | 29 | 100.0 |
| `src/services/AuditService.h` | 1 | 1 | 100.0 |
| `src/services/AuthService.cpp` | 52 | 52 | 100.0 |
| `src/services/BookingService.cpp` | 250 | 282 | 88.7 |
| `src/services/BookingService.h` | 2 | 2 | 100.0 |
| `src/services/TrainService.cpp` | 148 | 151 | 98.0 |
| `src/services/TrainService.h` | 2 | 2 | 100.0 |
| `src/services/UserService.cpp` | 67 | 73 | 91.8 |
| `src/services/UserService.h` | 1 | 1 | 100.0 |
| `src/util/Log.cpp` | 20 | 21 | 95.2 |
| `src/util/TlsConfig.cpp` | 13 | 18 | 72.2 |
| `src/util/TlsConfig.h` | 1 | 1 | 100.0 |
| `src/util/Validation.cpp` | 76 | 78 | 97.4 |

## Follow up

Prioritize untested error and rollback paths visible in the HTML report. A passing test suite is not equivalent to complete branch coverage.
