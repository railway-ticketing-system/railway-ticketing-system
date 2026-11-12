# Individual viva preparation

Prepared 2026-11-12 against `6b5464298c949806ebba0b7bd1d903c962b5255d`. Answers describe inspected implementation, not claims that the speaker personally wrote or tested everything. Use only the contribution statement appropriate to the person.

## Current verification evidence (2026-11-12)

A local verification run freshly built `build-submission` in Debug with GCC 16.2.1 and `RTRS_COVERAGE=ON` at application SHA `6b5464298c949806ebba0b7bd1d903c962b5255d`. [Unit Test Report](../reports/Unit_Test_Report.md): **201/201 tests, 49 suites**. [Line coverage](../reports/Code_Coverage_Report.md): **94.7% (1476/1558)**; [branch coverage](../reports/Branch_Coverage_Report.md): **46.7% (3162/6778)**; functions **100% (167/167)**. Coverage excludes `src/main.cpp`, tests, dependencies and browser code; executable compilation introduces no application change.

[System Validation Report](../reports/System_Validation_Report.md) and [raw JSON](../reports/system_validation.json): **122 passed, 0 failed**, run 2026-11-12T16:49:13.470921+00:00 to 16:49:15.211579+00:00. A real localhost **TLS 1.3** handshake with hostname verification and a temporary self-signed trust anchor passed. Controlled restart and forced process stop/relaunch after committed writes preserved checked state and passed SQLite integrity checks; this does not establish power-loss or interrupted-transaction durability. Separate batches of 20 searches and 20 two-passenger bookings, four clients each, measured p90 **0.002256s search / 0.006130s booking**. These are small local load measurements, not production-scale results.

[Browser Validation Report](../reports/Browser_Validation_Report.md): selected desktop 1366×900 registration/login/search and mobile 390×844 login/search/schedule/availability/preview/booking/cancellation flows passed, with viewport width checks. Desktop booking was not independently repeated; physical-device, complete accessibility and cross-browser coverage are not established. These current results do not validate historical snapshots. Human UAT, signatures and policy approval remain pending; no video is claimed created.

## Team identity and contribution evidence

| Person | Role / SRS USN | Verified evidence | Honest boundary |
|---|---|---|---|
| Deeptanshu Kumar | Developer / PES1UG24AM357 | Authored setup `8250f4a`, registration `4994d02`, login `f4b8f48`, logout `4120cf7`, profile `99367b1`; train/search RTRS-13–17, booking RTRS-18–24, cancellation RTRS-25–28, admin RTRS-29–32 and audit/security RTRS-33–40 commits. SRS upload `561cc2f`. | Git authorship supports attributed implementation and associated test code; it does not prove all manual verification or sole intellectual authorship. |
| Amar | Tester / PES1UG24AM340 | `704559d` (author Amar1919, 2026-09-29) added SAD and test-plan DOCX files; `9eb6371` updated README roles on the same date. GitHub lists Amar1919 as author of integration PRs #7–12. | Document uploads and integration activity are evidenced. Do not claim backend feature/test-code authorship from merges, or claim all test cases executed without dated execution records. |
| Aaryaprasad | Product Owner / PES1UG24AM331 | PO role in README and SRS (SRS spelling “Aryaprasad”). | No PO-authored implementation commit or acceptance/sign-off was established by inspected Git/PR records. Requirements/prioritization is the assigned role; personal work needs its own authentic artifact. |

Example truthful openings:

- **Deeptanshu:** “My recorded development work covers the C++ API, static browser client, SQLite persistence and automated tests in the story commits. I can trace booking/cancellation consistency and session authorization in the code.”
- **Amar:** “My evidenced repository contributions include adding the SAD/test-plan documents and opening integration PRs. My assigned role is testing. I will distinguish that plan and integration history from test execution evidence.” Add specific personally executed cases only with real records.
- **Aaryaprasad:** “My assigned role is Product Owner, covering requirements and priorities. The SRS identifies my role and USN. I will explain acceptance criteria and limitations; I am not claiming implementation commits or a recorded approval that the evidence does not contain.”

### Authentic Git/PR anchors

Read-only GitHub inspection on 2026-11-12 returned these actual records:

| Record | Author / merge time (UTC) | What it establishes |
|---|---|---|
| [PR #2](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/2) | deeptanshukumar / 2026-10-13T17:54:30Z | Setup integration |
| [PR #3](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/3) | deeptanshukumar / 2026-10-13T18:19:34Z | Registration integration |
| [PR #4](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/4) | deeptanshukumar / 2026-10-13T18:33:56Z | Login integration |
| [PR #5](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/5) | deeptanshukumar / 2026-10-15T06:45:10Z | Logout integration |
| [PR #6](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/6) | deeptanshukumar / 2026-10-15T06:50:02Z | Profile integration |
| [PR #7](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/7) | Amar1919 / 2026-10-15T12:53:18Z | Integration PR titled “Merge tested features from test into main”; title alone is not test evidence |
| [PR #8](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/8) | Amar1919 / 2026-10-20T09:14:05Z | Integration PR |
| [PR #9](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/9) | Amar1919 / 2026-10-29T03:51:11Z | Integration PR; actual PR number is 9, although local merge subject says “#18-#24” |
| [PR #10](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/10) | Amar1919 / 2026-10-29T06:31:11Z | Cancellation-era integration PR |
| [PR #11](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/11) | Amar1919 / 2026-11-03T12:05:49Z | Admin-era integration PR |
| [PR #12](https://github.com/railway-ticketing-system/railway-ticketing-system/pull/12) | Amar1919 / 2026-11-03T13:02:29Z | Final inspected main integration, merge `fb1b4d5` |

Commit dates and PR merge times are different facts. Do not alter either to make a neat sprint timeline. The original SRS is dated 06-09-2026; SAD 30-09-2026 and STP 02-10-2026, although the latter files were added in Git on 2026-09-29. Their blank approval fields do not establish approval. Current Markdown preparation is separate from those historical contributions and is not a new implementation commit.

## Deeptanshu: implementation questions and answers

**Q: What actually runs, and where are the layers?**  
A: CMake builds a C++17 `rtrs_server`. cpp-httplib serves JSON APIs and `web/`; the frontend is static HTML/CSS/plain JavaScript. `src/api/` handles requests, services enforce business rules, `src/db/` wraps SQLite/schema/seed. It retains layered separation but does not implement the SAD React/Express/MySQL/JWT stack. See `CMakeLists.txt`, `src/main.cpp` and [deviations](../DEVIATIONS.md).

**Q: How do passwords and sessions differ?**  
A: PasswordHasher uses PBKDF2-HMAC-SHA256 with a random 16-byte salt and default 600,000 iterations; runtime configuration accepts 100,000–10,000,000. Login returns `randomHex(32)`, a 32-random-byte opaque bearer token. Only its SHA-256 hash is stored in `sessions`; it expires in 24 hours. This is a database-backed session, not JWT. The browser uses `sessionStorage` and an Authorization header (`web/app.js`, `AuthService.cpp`).

**Q: What happens on logout, password change and role change?**  
A: Logout deletes the session hash. Password change revokes other sessions while retaining the calling token. Administrator routes resolve the current database role each time. Host-side `--promote-admin` changes role and deletes all sessions for that user, so login must be repeated. Registration always creates a passenger.

**Q: Is brute-force protection permanent or distributed?**  
A: Five failures trigger a 300-second lock. Attempts are in an in-process map with a mutex, so this is not a durable/distributed rate limiter. Unknown emails also undergo dummy hash verification and return generic failures (`AuthService.h/.cpp`).

**Q: Why can two users not receive the same active seat?**  
A: `Database::transaction` holds a recursive mutex and starts `BEGIN IMMEDIATE`. Confirmation rechecks inventory and assigns free seat numbers within that transaction. SQLite has a 5-second busy timeout, WAL is requested, and a trigger rejects duplicate seats for the same train/date/class across confirmed bookings. Booking, reservation and audit writes commit or roll back together. This is a consistency design with bounded test scenarios, not a claim of unlimited scale (`Database.cpp`, `Schema.cpp`, `BookingService.cpp`).

**Q: Does preview hold a seat or guarantee a fare?**  
A: No. Preview validates passenger/class/train and availability. Confirmation re-evaluates the submitted booking. Other bookings or admin edits can change the result before confirmation. The browser then shows the actual confirmed ticket.

**Q: What exactly is reserved?**  
A: Inventory is per train, travel date and class, not individual route segment. Search can match intermediate stops, but the booking payload has trainId/date/class/passengers and does not carry origin/destination segment fields. Fares are stored class fares per passenger, not distance pricing.

**Q: How are PNRs and ticket routes handled?**  
A: Booking generates a numeric PNR and checks uniqueness with a database unique constraint as a backstop. Ticket `GET /api/bookings/{value}` interprets value as PNR; cancellation paths interpret their value as internal booking ID. Knowing a PNR is insufficient: ticket access also checks ownership.

**Q: Explain cancellation precisely.**  
A: Only CONFIRMED bookings can cancel. It reads the current first ordered stop's departure/day offset, combines it with the booking date in UTC and rejects when remaining seconds are less than cutoffHours × 3600. Default cutoff is 24 hours; equality passes the comparison. Refund is integer `totalFare × percent / 100`, default 90%, rounded down. Cancellation updates status/time/refund, deletes its reservation rows and appends an audit event in one transaction. The second attempt fails; freed seats can be reused. No real payment is transferred. Policy is provisional, not PO-approved.

**Q: Can an administrator's edit affect a previously booked trip?**  
A: Stored booking fare/reservations are preserved, but cancellation reads the current train origin schedule. Train edits can therefore affect eligibility. Referenced classes cannot be removed and capacity cannot go below reservations or assigned seat numbers. Deactivation blocks new bookings but preserves old ticket/history/cancellation access. There is no exposed reactivation flow.

**Q: How is authorization failure presented?**  
A: Missing/invalid sessions return 401; a passenger using admin APIs gets 403. Another owner's booking yields the same 404 as a nonexistent booking to avoid disclosing it. Service cancellation also verifies user ID inside the transaction (`Routes.cpp`, `Authorization.h`, `BookingService.cpp`).

**Q: What input and security boundaries remain?**  
A: API input has server validation, enum/length/type checks, 64 KB body limit and bound SQL values. Passwords cannot contain ASCII controls. Date validation checks a real calendar date but does not reject past bookings. Names/stations use restricted characters. HTTPS is configurable, not automatic. Unit tests check configuration/headers; the separate fresh system runner verified an actual localhost TLS 1.3 handshake using a temporary trust anchor, not a deployed production certificate.

**Q: What is audited versus logged?**  
A: Append-only booking/cancellation audit events are transactionally recorded with identifiers/refund. Access logs contain UTC time/method/path/status/address; query strings, bodies and headers are excluded, credentials redacted and controls stripped. This is not proof of an external tamper-proof compliance archive (`AuditService.cpp`, `Log.cpp`).

## Amar: testing questions and answers

**Q: What is your strongest attributable contribution?**  
A: The Git records show my SAD/STP uploads and README role correction, and GitHub shows integration PRs I authored. I must support any specific manual execution claim with a real report/screenshots/date; merging developer-written tests does not make me their author.

**Q: How would you test a booking without relying only on the success screen?**  
A: Measure train/date/class availability A, preview and verify A unchanged, confirm and check PNR/seat/history plus A−n, cancel and verify status/time/refund plus A restored. Check another owner gets 404 and repeat cancel cannot restore again. Record actual results separately from this test design.

**Q: Which boundaries and negative cases matter?**  
A: Ages 0/1/120/121; passenger counts 0/1/6/7; malformed JSON/wrong types; duplicate account/train; stale preview with no remaining capacity; missing/revoked token; other-owner access; before/after cancellation cutoff; configured refund rounding; rollback after inventory failure. Existing automated examples are in `tests/test_booking_cancel.cpp`, `test_booking_validation.cpp`, `test_input_validation.cpp` and `test_security.cpp`. Exact cutoff timing is sensitive to the wall clock.

**Q: What do the concurrency and performance tests actually prove?**  
A: The defined concurrency test sends eight simultaneous confirmations for two seats through one TestServer and expects two successes/six conflicts with unique seats. The performance test has four threads making twenty search requests each and asserts sampled p90 below 3000 ms on local test data. That unit test does not measure booking latency, despite RTRS-34's commit title. Separately, the fresh system runner measured 20 searches and 20 two-passenger bookings with four clients per batch: p90 0.002256s and 0.006130s respectively. Neither scenario establishes production-scale load.

**Q: Can you say coverage or acceptance is complete?**  
A: Fresh reports establish 94.7% line coverage (1476/1558), 46.7% branch coverage (3162/6778) and 100% function coverage (167/167). Coverage instruments `src/` and excludes `src/main.cpp`, tests, dependencies and browser code. Real TLS/recovery have separate passing system evidence; they are outside that coverage percentage. No minimum percentage gate is configured. The STP's 100% planned-case execution target is an exit criterion, not a measured coverage result.

**Q: What current automated evidence exists?**  
A: GitHub metadata read on 2026-11-12 reports [CI run 41640804703](https://github.com/railway-ticketing-system/railway-ticketing-system/actions/runs/41640804703) succeeded, created 2026-11-03T13:02:32Z at merge SHA `fb1b4d5`. That is historical CI status. Fresh local reports now establish 201/201 tests across 49 suites and 122/122 system checks, plus selected browser flows and the numeric coverage above. System checks include TLS 1.3, controlled restart and forced-stop recovery after committed writes with integrity checks. These are separate from the old CI run and do not establish individual tester execution or human sign-off.

## Aaryaprasad: product questions and answers

**Q: What is the product scope?**  
A: Browser-based account, search/schedule, seat availability, booking/PNR/history, eligible cancellation with recorded refund, administrator maintenance and audit. Real railway operations, railway-provider integration and production payment settlement are outside the prototype scope.

**Q: What acceptance criteria would you use?**  
A: Confirmed booking has unique PNR/seat and correct inventory; only the owner can view/cancel; eligible cancellation records correct refund and restores inventory once; admin changes are protected; deactivated services reject new reservations. Separate these observable criteria from full STP exit requirements for security, performance, recovery and UI evidence.

**Q: Have you approved the architecture or refund defaults?**  
A: No approval is established by the artifacts inspected. The baseline documents have blank signature fields. The code uses provisional 24h/90% defaults. A real dated PO decision would be needed to claim acceptance; this Markdown does not create one.

**Q: How do you explain the architecture deviation?**  
A: The original SAD specifies React/Node/Express/MySQL-like relational SQL/JWT. The delivered system uses static JavaScript/C++17/SQLite/opaque sessions. Layering and core workflows remain, but the technology baseline has changed. Simpler packaging is an observed consequence, not a documented historical rationale or approval.

**Q: What would you prioritize next?**  
A: Obtain actual policy disposition and manual acceptance evidence, review remaining production-load, power-loss, accessibility and broader browser gaps beyond the selected passing checks, and decide whether date/segment handling and deployment scope meet the intended product. This is proposed future work, not an approved backlog or delivered feature.
