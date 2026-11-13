# Unit Test Report

Generated 2026-11-12T16:45:25.898236+00:00. Application source: `6b5464298c949806ebba0b7bd1d903c962b5255d`.

Environment: Linux-7.2.14-200.fc44.x86_64-x86_64-with-glibc2.43; g++ (GCC) 16.2.1 20261029 (Red Hat 16.2.1-3); gcovr 8.8.

GoogleTest executed **201 tests** in **49 suites**: **0 failures**, **0 errors**, 0 disabled; reported duration 1.715 seconds. This binary includes unit, service, API integration, concurrency and search performance tests; the total is not a count of distinct Test Plan cases.

Built freshly with CMake Debug, GCC and `RTRS_COVERAGE=ON`. Evidence: [GoogleTest XML](unit-tests.xml), [CTest JUnit](ctest-results.xml), [system validation](System_Validation_Report.md). Test hashes use reduced PBKDF2 iterations for speed; the system runner separately exercises the executable configuration.

| Suite | Tests | Failures | Time seconds |
|---|---:|---:|---:|
| AdminAuthApi | 3 | 0 | 0.044 |
| AdminTrainCreateApi | 6 | 0 | 0.082 |
| AdminTrainUpdateApi | 5 | 0 | 0.099 |
| AdminTrainDeactivateApi | 4 | 0 | 0.078 |
| LoginApi | 5 | 0 | 0.046 |
| LogoutApi | 2 | 0 | 0.018 |
| RegisterApi | 6 | 0 | 0.045 |
| AuditApi | 3 | 0 | 0.049 |
| AuthServiceTest | 10 | 0 | 0.02 |
| AvailabilityTest | 4 | 0 | 0.005 |
| AvailabilityApi | 3 | 0 | 0.019 |
| BookingAvailabilityApi | 5 | 0 | 0.05 |
| BookingCancelApi | 6 | 0 | 0.074 |
| BookingRefundApi | 3 | 0 | 0.04 |
| BookingRestoreApi | 3 | 0 | 0.047 |
| BookingCancelSecurityApi | 3 | 0 | 0.044 |
| BookingConfirmApi | 7 | 0 | 0.07 |
| BookingHistoryApi | 5 | 0 | 0.061 |
| BookingPnrApi | 4 | 0 | 0.102 |
| BookingPreviewApi | 8 | 0 | 0.072 |
| BookingTicketApi | 4 | 0 | 0.048 |
| BookingValidationApi | 5 | 0 | 0.05 |
| ConcurrencyApi | 1 | 0 | 0.041 |
| ConcurrencyDatabase | 1 | 0 | 0.001 |
| Database | 4 | 0 | 0. |
| SafeErrorsApi | 6 | 0 | 0.033 |
| FaresTest | 2 | 0 | 0.003 |
| FaresApi | 3 | 0 | 0.017 |
| Health | 1 | 0 | 0.078 |
| ValidationInput | 1 | 0 | 0. |
| InputValidationApi | 6 | 0 | 0.057 |
| LoggingUtil | 3 | 0 | 0. |
| PasswordHasher | 4 | 0 | 0. |
| Random | 1 | 0 | 0. |
| PerformanceApi | 1 | 0 | 0.064 |
| ProfileServiceTest | 4 | 0 | 0.009 |
| ProfileApiTest | 6 | 0 | 0.069 |
| SecurityApi | 6 | 0 | 0.057 |
| PasswordHasherSecurity | 1 | 0 | 0. |
| TrainFilterTest | 9 | 0 | 0.015 |
| FilterApi | 2 | 0 | 0.012 |
| TrainScheduleTest | 2 | 0 | 0.003 |
| ScheduleApi | 3 | 0 | 0.017 |
| TrainSearchTest | 9 | 0 | 0.012 |
| SearchApi | 4 | 0 | 0.023 |
| TlsConfigTest | 3 | 0 | 0. |
| TransportSecurityTest | 2 | 0 | 0.01 |
| UserServiceTest | 6 | 0 | 0.009 |
| Validation | 6 | 0 | 0. |

## Reproduction

Run `python3 scripts/submission/collect_reports.py --build-dir build-submission --jobs 4` with CMake, GCC, OpenSSL, gcovr 8.8 and Python available. An initial dependency download requires network access.

## Interpretation

These results validate the selected scenarios on one local Linux machine. They do not establish production readiness, hosted deployment, penetration-test certification, or human UAT acceptance. See the System Validation Report and submission checklist for remaining human actions.
