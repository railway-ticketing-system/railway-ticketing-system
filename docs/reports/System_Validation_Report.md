# System Validation Report

Outcome: **PASS** — 122 passed, 0 failed.

Run: 2026-11-12T16:49:13.470921+00:00 to 2026-11-12T16:49:15.211579+00:00
Source SHA: `6b5464298c949806ebba0b7bd1d903c962b5255d` (finish `6b5464298c949806ebba0b7bd1d903c962b5255d`)
Binary: `/home/deeptanshukumar/Downloads/SE-Railway Ticket Reservation System/build-submission/rtrs_server`
Binary SHA-256: `8437a1bb68a34e759244665e3c4deb2e565bc843e6ba2d53e46cd6e2a724f451`
Travel date: 2026-11-15 (system local date +3 days).

## Environment

```json
{
  "platform": "Linux-7.2.14-200.fc44.x86_64-x86_64-with-glibc2.43",
  "python": "3.13.13 | packaged by Anaconda, Inc. | (main, Apr 14 2026, 06:19:41) [GCC 14.3.0]",
  "openssl": "OpenSSL 3.6.3 9 Jun 2026",
  "cpuCount": 18,
  "timezone": [
    "IST",
    "IST"
  ],
  "pythonSqliteVersion": "3.51.2"
}
```

The Python SQLite version above describes the runner’s `sqlite3` module, not the server. The application bundles SQLite 3.46.1; this is not a runtime server-version measurement.

## Method and scope

- **isolation:** Fresh TemporaryDirectory under /tmp, dedicated SQLite DB; inherited RTRS_* settings removed; ephemeral ports; no production DB
- **transport:** Actual server subprocess and localhost HTTP/HTTPS sockets; no mocks
- **load:** 20 search + 20 two-passenger bookings, separately, four concurrent clients, fresh connection per request, nearest-rank p90 <3 seconds each; connection through full response latency
- **recovery:** Controlled restart and abrupt process kill/relaunch after committed writes, same database plus read-only SQLite integrity check; not power-loss or interrupted-transaction durability
- **credentials:** Random runtime password; request bodies and auth headers never logged by runner; response secrets redacted

## Measured performance

| Operation | Requests | Clients | p90 (s) | Max (s) | Threshold |
|---|---:|---:|---:|---:|---|
| search | 20 | 4 | 0.002256 | 0.002906 | <3 s |
| booking | 20 | 4 | 0.006130 | 0.007966 | <3 s |

Detailed expected/actual bodies, raw latency samples and per-step timestamps are preserved in [raw JSON evidence](system_validation.json). Check IDs below identify the corresponding 1-based entries in its `steps` array.

## Failures and limitations

No failed assertions observed in executed checks.
- Small local load only; no sustained stress or internet latency conclusions
- TLS uses a temporary self-signed trust anchor; does not validate production certificates
- Admin audit API records booking events; train changes verified via read-back and restart
- No UI, screenshot, coverage or unit-test results claimed by this independent runner

## Check results

| Check ID | Name | Status |
|---|---|---|
| SV-001 | Start HTTP server | PASS |
| SV-002 | Register owner@example.test | PASS |
| SV-003 | Registration cannot assign admin: owner@example.test | PASS |
| SV-004 | Register other@example.test | PASS |
| SV-005 | Registration cannot assign admin: other@example.test | PASS |
| SV-006 | Register admin@example.test | PASS |
| SV-007 | Registration cannot assign admin: admin@example.test | PASS |
| SV-008 | Login owner@example.test | PASS |
| SV-009 | Login other@example.test | PASS |
| SV-010 | Login admin@example.test | PASS |
| SV-011 | Anonymous profile denied | PASS |
| SV-012 | Profile update | PASS |
| SV-013 | Profile read | PASS |
| SV-014 | Profile update and role protection | PASS |
| SV-015 | Future Pune to Mumbai search | PASS |
| SV-016 | Seeded route results | PASS |
| SV-017 | Schedule | PASS |
| SV-018 | Ordered route stops | PASS |
| SV-019 | Seat inventory | PASS |
| SV-020 | Class fares | PASS |
| SV-021 | Fare consistency | PASS |
| SV-022 | Booking preview | PASS |
| SV-023 | Preview fare arithmetic | PASS |
| SV-024 | Seat inventory | PASS |
| SV-025 | Seat inventory | PASS |
| SV-026 | Preview does not reserve | PASS |
| SV-027 | Create booking | PASS |
| SV-028 | Confirmed ticket and assigned unique seats | PASS |
| SV-029 | Seat inventory | PASS |
| SV-030 | Seat inventory | PASS |
| SV-031 | Seat decrement | PASS |
| SV-032 | Owner ticket | PASS |
| SV-033 | Ticket matches confirmed booking | PASS |
| SV-034 | Owner history | PASS |
| SV-035 | History contains booking | PASS |
| SV-036 | Cross-user denied GET | PASS |
| SV-037 | Cross-user denied GET/cancellation | PASS |
| SV-038 | Cross-user denied POST/cancel | PASS |
| SV-039 | Cross-user denied PUT/cancel | PASS |
| SV-040 | Other user history | PASS |
| SV-041 | History isolation | PASS |
| SV-042 | Refund quote | PASS |
| SV-043 | Default cancellation policy | PASS |
| SV-044 | Cancel booking | PASS |
| SV-045 | Refund recorded and cancellation timestamp | PASS |
| SV-046 | Repeat cancellation rejected | PASS |
| SV-047 | Seat inventory | PASS |
| SV-048 | Seats restored exactly once | PASS |
| SV-049 | Passenger admin denial /api/admin/me | PASS |
| SV-050 | Anonymous admin denial /api/admin/me | PASS |
| SV-051 | Passenger admin denial /api/admin/trains | PASS |
| SV-052 | Anonymous admin denial /api/admin/trains | PASS |
| SV-053 | Passenger admin denial /api/admin/audit | PASS |
| SV-054 | Anonymous admin denial /api/admin/audit | PASS |
| SV-055 | Passenger admin write denied | PASS |
| SV-056 | Stopped-server admin promotion CLI | PASS |
| SV-057 | Start HTTP server | PASS |
| SV-058 | Promotion revokes old session | PASS |
| SV-059 | Login admin@example.test | PASS |
| SV-060 | Admin role | PASS |
| SV-061 | Promoted role | PASS |
| SV-062 | Admin create train | PASS |
| SV-063 | Admin update train | PASS |
| SV-064 | Admin update reflected | PASS |
| SV-065 | Created train public visibility | PASS |
| SV-066 | Managed train searchable | PASS |
| SV-067 | Admin deactivate train | PASS |
| SV-068 | Train inactive | PASS |
| SV-069 | Inactive train excluded from search | PASS |
| SV-070 | No inactive results | PASS |
| SV-071 | Inactive schedule unavailable | PASS |
| SV-072 | Admin audit | PASS |
| SV-073 | Booking audit correlates actor, booking, refund | PASS |
| SV-074 | Malformed JSON | PASS |
| SV-075 | Malformed JSON safe error | PASS |
| SV-076 | Invalid booking passengers | PASS |
| SV-077 | Invalid booking passengers safe error | PASS |
| SV-078 | Invalid date | PASS |
| SV-079 | Invalid date safe error | PASS |
| SV-080 | Unknown endpoint | PASS |
| SV-081 | Unknown endpoint safe error | PASS |
| SV-082 | Invalid audit limit | PASS |
| SV-083 | Invalid audit limit safe error | PASS |
| SV-084 | Anonymous booking | PASS |
| SV-085 | Anonymous booking safe error | PASS |
| SV-086 | Unknown account login | PASS |
| SV-087 | Wrong password login | PASS |
| SV-088 | Login does not enumerate accounts | PASS |
| SV-089 | search small-load p90 | PASS |
| SV-090 | booking small-load p90 | PASS |
| SV-091 | Concurrent booking unique PNRs and seats | PASS |
| SV-092 | Pre-restart history | PASS |
| SV-093 | Start HTTP server | PASS |
| SV-094 | Login owner@example.test | PASS |
| SV-095 | Persisted profile | PASS |
| SV-096 | Persisted bookings | PASS |
| SV-097 | Restart persistence exact history/profile | PASS |
| SV-098 | Seat inventory | PASS |
| SV-099 | Seat inventory | PASS |
| SV-100 | Restart inventory persists | PASS |
| SV-101 | Login admin@example.test | PASS |
| SV-102 | Persisted admin train | PASS |
| SV-103 | Persisted audit | PASS |
| SV-104 | Admin state and audit survive restart | PASS |
| SV-105 | Abrupt process stop | PASS |
| SV-106 | Start HTTP server | PASS |
| SV-107 | Login owner@example.test | PASS |
| SV-108 | Crash-recovered history | PASS |
| SV-109 | Committed bookings survive abrupt stop | PASS |
| SV-110 | Seat inventory | PASS |
| SV-111 | Seat inventory | PASS |
| SV-112 | Crash-recovered inventory | PASS |
| SV-113 | Recovered SQLite integrity | PASS |
| SV-114 | Logout | PASS |
| SV-115 | Logged-out session denied | PASS |
| SV-116 | Generate temporary TLS certificate | PASS |
| SV-117 | Start HTTPS server | PASS |
| SV-118 | Real verified TLS handshake | PASS |
| SV-119 | HTTPS health and security headers | PASS |
| SV-120 | Login owner@example.test | PASS |
| SV-121 | Authenticated profile over real TLS | PASS |
| SV-122 | Server logs contain no generated credentials | PASS |
