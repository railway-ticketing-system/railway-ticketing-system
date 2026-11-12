# Implementation deviations and evidence limits

Recorded 2026-11-12 from README, extracted SRS/PDF and SAD/STP DOCX text, source, local Git and read-only GitHub metadata. This record supplements the originals without editing or backdating them. It does not approve deviations or certify acceptance.

## Baseline and dates

| Artifact / event | Actual date or identity | Interpretation |
|---|---|---|
| SRS v1.0 | 06-09-2026; Draft for Mini Project Submission | Requirements baseline; role/USN source |
| SAD v1.0 | 30-09-2026; Draft for Mini Project Submission | Original architecture baseline; signature fields blank |
| STP v1.0 | 02-10-2026; Draft for Mini Project Submission | Planned verification and exit criteria; signature fields blank |
| SAD/STP upload | Git `704559d`, Amar1919, 2026-09-29T18:52:21+05:30 | File-upload evidence, not approval or test execution; embedded dates differ and are preserved |
| Inspected checkout | `6b5464298c949806ebba0b7bd1d903c962b5255d`, `deliverables/final-submission` | Documentation implementation baseline; commit date 2026-11-03T18:24:41+05:30 |
| Later inspected main integration | `fb1b4d55957ab8022e136396d656bec870c08375`, PR #12 merged 2026-11-03T13:02:29Z | Separate merge identity; do not describe checkout HEAD as this SHA |
| This supplemental record | 2026-11-12 | Inspection/preparation date, not a test or approval date |

The STP schedules activities on 02-10-2026, 03-10-2026, 04-10-2026, 05-10-2026 through 07-10-2026, 08-10-2026 and 09-10-2026. These remain planned dates; subsequent October/November commits do not retroactively establish completion on those dates.

## Technology and interface deviations

| ID | Original SAD baseline | Actual inspected implementation / evidence | Consequence and disposition |
|---|---|---|---|
| D-01 | React UI; CSS/Tailwind as applicable | Static HTML/CSS/plain JavaScript in `web/`, mounted by cpp-httplib in `src/main.cpp` | No React build/runtime; framework-specific diagrams and React integration descriptions are obsolete. Browser validation still needs direct evidence. No change approval found. |
| D-02 | Node.js/Express application tier | C++17 compiled server; cpp-httplib routes, nlohmann/json, service classes; `CMakeLists.txt`, `src/api/`, `src/services/` | Layered architecture persists, but runtime, tools and build instructions differ. No Node/Express backend is delivered. Reduced runtime packaging is observable; historical rationale was not documented in the evidence inspected. |
| D-03 | Relational SQL DBMS, e.g. MySQL | Embedded SQLite amalgamation; schema initialized in C++; file path via RTRS_DB; `src/db/` | MySQL is an example in the SAD, not an established deployed requirement. Actual storage is a local SQLite file with foreign keys, requested WAL, busy timeout and immediate write transactions. No external DB server; no proof of production/distributed scale or managed recovery. |
| D-04 | JWT authentication technology (some API wording allows JWT/session) | Random opaque bearer tokens; SHA-256 hashes in SQLite sessions, 24-hour TTL; frontend sessionStorage; `AuthService.cpp`, `web/app.js` | Database-backed revocation replaces signed JWT claims. Logout invalidates token server-side; roles re-read from DB; password change revokes other sessions. Browser token storage is not an HttpOnly cookie. |
| D-05 | Illustrative API with journeyDate and generic booking IDs | Search/availability use `date`; preview has a separate endpoint. Ticket GET `/api/bookings/{PNR}` resolves PNR, while cancellation endpoints resolve internal ID; `Routes.cpp` | Clients must follow actual route semantics, not copy SAD placeholders. Other-owner booking access is 404 rather than the SAD's generic ownership/role 403 description; admin role denial is 403. |
| D-06 | Postman and React/backend/SQL integration testing described | GoogleTest service/API tests, CTest, gcov/gcovr, cppcheck, CodeQL and Docker workflows | Test infrastructure differs from planned tooling. CI job success cannot establish manual Postman runs or React/browser integration execution. |

The C++ implementation still provides presentation/API/service/persistence separation. These substitutions should not be framed as abandonment of all SRS workflows, nor as an approved architecture revision. No recorded decision-maker, approval date or retrospective design rationale is invented here.

## Cancellation, inventory and product limits

| ID | Actual rule or limitation | Source / implication |
|---|---|---|
| D-07 | Default cancellation cutoff 24 hours; refund 90%; configurable via RTRS_CANCEL_CUTOFF_HOURS and RTRS_REFUND_PERCENT | README and `src/main.cpp` explicitly expose project policy; README says final team policy decision remains pending. SAD also lists policy finalization as open. These are provisional defaults, not railway rules or approved PO policy. |
| D-08 | Eligibility uses the booking date plus the current train's first stop departure and day offset, interpreted as UTC | `BookingService::cancellable`. Rejects when remaining seconds are strictly less than cutoff hours; equality is eligible at the instant compared. It does not use a boarding-segment departure or a departure snapshot stored at booking. Admin schedule edits may change existing booking eligibility. UI times must not be casually labelled IST. |
| D-09 | Only whole CONFIRMED bookings cancel; repeat cancel rejected; timestamp/refund recorded and reservation rows removed atomically | `BookingService::cancel`, `tests/test_booking_cancel.cpp`. No passenger-level partial cancellation or settlement workflow. Refund is floor(total stored booking fare × configured percent / 100) in whole rupees; no payment-provider transfer. A quoted eligible result can become ineligible before confirmation. |
| D-10 | Inventory and fares are per train/date/class; booking payload omits source/destination segment | `TrainService.cpp`, `BookingService.cpp`, `web/app.js`. Search matches ordered route stops, but no segment-level seat reuse or distance-dependent fare allocation is implemented. Availability preview does not hold seats. |
| D-11 | Real calendar date validation permits past dates | `Validation::isValidDate`, `BookingService` validation. No future-journey rule or operating-day calendar is established. Seed trains are synthetic recurring records, not live railway data. Use a future date for the main cancellation demo. |
| D-12 | Deactivation is persistent, not deletion, and editing does not reactivate | `AdminTrainService.cpp`, README, admin tests. New bookings/search are blocked; old tickets/history and eligible cancellation persist. No exposed reactivation endpoint is established. Referenced classes and occupied capacity have edit conflict protections. |
| D-13 | HTTPS is opt-in; local default is HTTP | `src/main.cpp`, `TlsConfig.cpp`. Both PEM certificate/key paths are required for direct TLS. Proxy deployments must configure forwarded-protocol trust correctly. Header/configuration tests alone do not prove a deployed trusted TLS handshake. |
| D-14 | Login attempt lockout is process-local; frontend reload starts at login | `AuthService` attempts map and `web/app.js` initial show call. Not a distributed/durable abuse control or automatic UI session restoration. Persisted sessions still have server-side TTL and revocation. |

Neither real railway backend integration nor production payment settlement is promised by this prototype. Absence of those integrations is consistent with stated academic exclusions; it is not evidence a payment feature was completed.

## Test and coverage evidence boundaries

Updated 2026-11-12 with fresh verification performed by the parent workflow. The earlier statement that no build/tests/coverage had been performed is superseded by the reports below. The fresh `build-submission` build used CMake Debug, `RTRS_COVERAGE=ON`, GCC 16.2.1 (Red Hat 16.2.1-3) and gcovr 8.8 against application source `6b5464298c949806ebba0b7bd1d903c962b5255d`. Report generation timestamp: 2026-11-12T16:45:25.898236+00:00. Compilation of the main executable introduces no application change.

| Fresh verification | Recorded result | Evidence |
|---|---|---|
| Automated tests | 201/201 passed across 49 GoogleTest suites; 0 failures, 0 errors, 0 disabled | [Unit Test Report](reports/Unit_Test_Report.md), [GoogleTest XML](reports/unit-tests.xml), [CTest JUnit XML](reports/ctest-results.xml) |
| Line coverage | 94.7% — 1476/1558 measured lines | [Code Coverage Report](reports/Code_Coverage_Report.md), [line summary](reports/line-coverage.txt) |
| Branch coverage | 46.7% — 3162/6778 measured branches | [Branch Coverage Report](reports/Branch_Coverage_Report.md) |
| Function coverage | 100% — 167/167 measured functions | [summary JSON](reports/coverage-summary.json), [function HTML](reports/coverage.functions.html) |
| Reproducible context and detailed coverage | GCC/gcov instrumentation; project `src/` files and headers; `src/main.cpp`, tests and downloaded dependencies excluded | [execution metadata](reports/execution-metadata.json), [raw coverage JSON](reports/coverage.json), [interactive HTML](reports/coverage.html), [collection log](reports/coverage-collection.log) |

The 201 tests include unit, service, API integration, concurrency and search-performance scenarios; they are not 201 separately executed STP cases. Function coverage does not imply complete line/branch or requirements coverage. Compiler-generated C++ exception/cleanup edges remain in the branch denominator; no exception/unreachable-branch exclusions are applied. Coverage collection tolerates negative-hit parse artifacts as recorded in its log. Browser JavaScript/HTML/CSS, startup/CLI and deployment are outside these percentages.

[System Validation Report](reports/System_Validation_Report.md) and [raw JSON](reports/system_validation.json): **122 passed, 0 failed**, run 2026-11-12T16:49:13.470921+00:00 to 16:49:15.211579+00:00. A real localhost **TLS 1.3** handshake with hostname verification and a temporary self-signed trust anchor passed. Controlled restart and forced process stop/relaunch after committed writes preserved checked state and passed SQLite integrity checks; this does not establish power-loss or interrupted-transaction durability. Separate batches of 20 searches and 20 two-passenger bookings, four clients each, measured p90 **0.002256s search / 0.006130s booking**. These are small local load measurements, not production-scale results.

[Browser Validation Report](reports/Browser_Validation_Report.md): selected desktop 1366×900 registration/login/search and mobile 390×844 login/search/schedule/availability/preview/booking/cancellation flows passed, with viewport width checks. Desktop booking was not independently repeated; physical-device, complete accessibility and cross-browser coverage are not established. These current results do not validate historical snapshots. Human UAT, signatures and policy approval remain pending; no video is claimed created.

The original baseline artifacts and original diagrams are preserved in Git. The official files are being revised to version 2 today, 2026-11-12; this is a current revision activity, not a retroactive change to the v1 dates or evidence of approval. This update does not claim those revisions are finished.

Demo readers should use these current report links alongside the [live demo runbook](demo/LIVE_DEMO_RUNBOOK.md), [viva preparation](demo/VIVA_PREP.md) and [video handoff](demo/VIDEO_HANDOFF.md). Their earlier preparation-time “no tests performed” wording is superseded for current verification by this evidence section; the demo runbook, viva preparation and video handoff now also link current evidence. No video completion or manual acceptance sign-off is claimed.

Historical read-only GitHub metadata inspected on 2026-11-12 remains separate from the fresh local evidence:

- [CI run 41640804703](https://github.com/railway-ticketing-system/railway-ticketing-system/actions/runs/41640804703): success, created 2026-11-03T13:02:32Z, SHA `fb1b4d5`.
- [CD run 41640804787](https://github.com/railway-ticketing-system/railway-ticketing-system/actions/runs/41640804787): success at the same SHA and creation time; workflow status does not verify deployment on the demo machine.
- [CodeQL run 42604246430](https://github.com/railway-ticketing-system/railway-ticketing-system/actions/runs/42604246430): success, created 2026-11-09T10:51:04Z, SHA `fb1b4d5`; successful scan does not prove zero vulnerabilities.

| Area | Established evidence | Remaining boundary / current status |
|---|---|---|
| Coverage gate | Fresh numeric line/branch/function results above; CI reporting/upload configuration | No fail-under coverage percentage gate is configured. 94.7% line and 46.7% branch coverage are different measures; neither establishes full acceptance. |
| Performance | The passing GoogleTest search case uses four threads × twenty local requests and asserts sampled p90 <3000 ms | That case does not measure booking latency despite RTRS-34's commit title. Separate system-runner small-load p90 is 0.002256s search and 0.006130s booking (20 requests per operation, four clients); no production-scale claim. |
| Concurrency | Passing eight-buyers/two-seats API scenario and duplicate-seat trigger test | Shared TestServer/database wrapper serializes DB operations with its mutex. Multi-process/file-backed stress and deployment-scale contention are not established by this scenario. |
| Recovery | Passing transaction rollback/commit and cancellation inventory-failure rollback tests | Controlled restart and forced-stop/relaunch after committed writes passed with integrity checks. Power-loss, interrupted-transaction durability and complete recovery acceptance remain unestablished. |
| Transport | Passing TLS configuration/header tests | Actual localhost TLS 1.3 handshake and authenticated HTTPS passed with a temporary trust anchor; production certificate/proxy validation remains distinct. |
| UI/accessibility | Selected desktop registration/login/search and mobile through booking/cancellation passed; see linked Browser Validation Report | Desktop booking was not separately repeated. No physical-device, full accessibility, cross-browser or human UAT acceptance is established. API tests do not exercise the DOM. |
| Test fixture | In-memory DB and registerRoutes iteration argument 1000 in `tests/TestServer.h` | Different from production main/configuration, whose accepted iteration range starts at 100,000 with default 600,000. API-fixture timing is not a production authentication benchmark. |
| Acceptance | Fresh automated results and coverage artifacts are available | Full executed RTM, human acceptance, defect closure and signed dispositions remain separate requirements. Blank approvals and merged PRs do not establish them. |

## Attribution and unresolved disposition

Assigned roles remain Deeptanshu developer, Amar tester and Aaryaprasad PO. SRS USNs are respectively PES1UG24AM357, PES1UG24AM340 and PES1UG24AM331; its PO spelling is Aryaprasad. Implementation story commits are attributed to Deeptanshu. Amar's SAD/STP upload, role edit and integration PR authorship are separately evidenced. No PO code contribution or approval is inferred from the assigned role. See [individual viva evidence](demo/VIVA_PREP.md) for exact anchors.

Outstanding matters are to obtain an authentic technology/policy disposition, perform and record demo/manual acceptance checks, and collect revision-specific coverage/booking-performance/recovery/browser evidence where required. These are recorded gaps and possible follow-up work, not tasks certified as done or approvals requested/granted by this file.

Related Markdown: [live demo runbook](demo/LIVE_DEMO_RUNBOOK.md), [viva preparation](demo/VIVA_PREP.md), [friend's 60s/120s recording handoff](demo/VIDEO_HANDOFF.md).
