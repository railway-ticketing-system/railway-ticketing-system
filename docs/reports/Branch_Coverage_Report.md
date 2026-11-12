# Branch Coverage Report

Generated 2026-11-12T16:45:25.898236+00:00. Application source: `6b5464298c949806ebba0b7bd1d903c962b5255d`.

Environment: Linux-7.2.14-200.fc44.x86_64-x86_64-with-glibc2.43; g++ (GCC) 16.2.1 20261029 (Red Hat 16.2.1-3); gcovr 8.8.

**46.7%**: **3162 / 6778** measured branchs covered.

## Scope and method

GCC/gcov instrumentation with gcovr, restricted to project `src/` files and headers; `src/main.cpp`, tests and downloaded dependencies are excluded. The executable startup, CLI and real TLS path are additionally exercised by the system validation runner but are not included in this coverage percentage. No exclusions for exception branches or unreachable branches are applied. Compiler-generated C++ exception/cleanup edges remain in the branch denominator. No minimum percentage was specified in the five course PDFs.

Negative-hit parse handling matches the existing CI workaround: `--gcov-ignore-parse-errors negative_hits.warn_once_per_file`. The collection log is included as `coverage-collection.log`; inspect warnings when regenerating.

[Interactive source coverage](coverage.html) · [Raw coverage JSON](coverage.json) · [Summary JSON](coverage-summary.json)

| File | Covered | Total | Percent |
|---|---:|---:|---:|
| `src/api/AdminRoutes.cpp` | 395 | 1014 | 39.0 |
| `src/api/Authorization.h` | 40 | 86 | 46.5 |
| `src/api/Routes.cpp` | 998 | 2352 | 42.4 |
| `src/auth/PasswordHasher.cpp` | 79 | 140 | 56.4 |
| `src/auth/PasswordHasher.h` | 0 | 0 | N/A |
| `src/db/Database.cpp` | 42 | 80 | 52.5 |
| `src/db/Schema.cpp` | 67 | 138 | 48.6 |
| `src/db/Seed.cpp` | 64 | 142 | 45.1 |
| `src/services/AdminTrainService.cpp` | 373 | 724 | 51.5 |
| `src/services/AdminTrainService.h` | 0 | 0 | N/A |
| `src/services/AuditService.cpp` | 30 | 66 | 45.5 |
| `src/services/AuditService.h` | 0 | 0 | N/A |
| `src/services/AuthService.cpp` | 91 | 178 | 51.1 |
| `src/services/BookingService.cpp` | 370 | 792 | 46.7 |
| `src/services/BookingService.h` | 0 | 0 | N/A |
| `src/services/TrainService.cpp` | 263 | 464 | 56.7 |
| `src/services/TrainService.h` | 0 | 0 | N/A |
| `src/services/UserService.cpp` | 119 | 272 | 43.8 |
| `src/services/UserService.h` | 0 | 0 | N/A |
| `src/util/Log.cpp` | 31 | 58 | 53.4 |
| `src/util/TlsConfig.cpp` | 21 | 34 | 61.8 |
| `src/util/TlsConfig.h` | 3 | 4 | 75.0 |
| `src/util/Validation.cpp` | 176 | 234 | 75.2 |

## Follow up

Prioritize untested error and rollback paths visible in the HTML report. A passing test suite is not equivalent to complete branch coverage.
