# Final live demo runbook

Prepared 2026-11-12 by repository inspection. Implementation baseline: `6b5464298c949806ebba0b7bd1d903c962b5255d`, branch `deliverables/final-submission`. “Final” describes this runbook's intended use, not acceptance or a successful rehearsal. Fresh local build, test, system and selected browser evidence is linked below. The steps remain a repeatable presenter procedure; no recording or human acceptance is claimed.

## Current verification evidence (2026-11-12)

A local verification run freshly built `build-submission` in Debug with GCC 16.2.1 and `RTRS_COVERAGE=ON` at application SHA `6b5464298c949806ebba0b7bd1d903c962b5255d`. [Unit Test Report](../reports/Unit_Test_Report.md): **201/201 tests, 49 suites**. [Line coverage](../reports/Code_Coverage_Report.md): **94.7% (1476/1558)**; [branch coverage](../reports/Branch_Coverage_Report.md): **46.7% (3162/6778)**; functions **100% (167/167)**. Coverage excludes `src/main.cpp`, tests, dependencies and browser code; executable compilation introduces no application change.

[System Validation Report](../reports/System_Validation_Report.md) and [raw JSON](../reports/system_validation.json): **122 passed, 0 failed**, run 2026-11-12T16:49:13.470921+00:00 to 16:49:15.211579+00:00. A real localhost **TLS 1.3** handshake with hostname verification and a temporary self-signed trust anchor passed. Controlled restart and forced process stop/relaunch after committed writes preserved checked state and passed SQLite integrity checks; this does not establish power-loss or interrupted-transaction durability. Separate batches of 20 searches and 20 two-passenger bookings, four clients each, measured p90 **0.002256s search / 0.006130s booking**. These are small local load measurements, not production-scale results.

[Browser Validation Report](../reports/Browser_Validation_Report.md): selected desktop 1366×900 registration/login/search and mobile 390×844 login/search/schedule/availability/preview/booking/cancellation flows passed, with viewport width checks. Desktop booking was not independently repeated; physical-device, complete accessibility and cross-browser coverage are not established. These current results do not validate historical snapshots. Human UAT, signatures and policy approval remain pending; no video is claimed created.

## People and scope

Deeptanshu Kumar is developer (PES1UG24AM357), Amar is tester (PES1UG24AM340), and Aaryaprasad is Product Owner (PES1UG24AM331). The SRS spells the PO name “Aryaprasad”; the current README/user spelling is Aaryaprasad. USNs are extracted from the SRS, not inferred from GitHub accounts.

Demonstrate the local C++17 API, static HTML/CSS/JavaScript UI, SQLite persistence and opaque bearer sessions. Use synthetic data only. The friend handles video separately using [VIDEO_HANDOFF.md](VIDEO_HANDOFF.md); this runbook requires no video.

## Setup before the audience arrives

Prerequisites: CMake 3.20+, C/C++ compiler with C++17 support, OpenSSL development headers/libraries, thread support, browser; curl for optional API checks. Initial configure needs internet for pinned cpp-httplib, nlohmann/json, SQLite amalgamation and GoogleTest downloads. SQLite is built by CMake; a separate database server is unnecessary. Allow download/build time before the presentation.

Run from the repository root:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j2
ctest --test-dir build --output-on-failure
```

Record the actual command output, date, revision and failures if executing these later. These commands reproduce the fresh verification linked above; record results for any new rehearsal rather than substituting historical CI status.

Use a disposable database outside the working tree. In the same terminal, create a new directory for each rehearsal:

```bash
export RTRS_DEMO_DIR="$(mktemp -d /tmp/rtrs-demo.XXXXXX)"
export RTRS_DB="$RTRS_DEMO_DIR/demo.sqlite"
export RTRS_WEB_DIR="$PWD/web"
export RTRS_PORT=8080
export RTRS_CANCEL_CUTOFF_HOURS=24
export RTRS_REFUND_PERCENT=90
export RTRS_PBKDF2_ITERATIONS=600000
unset RTRS_TLS_CERT RTRS_TLS_KEY
./build/rtrs_server
```

Open `http://localhost:8080`. In another terminal, `curl -i http://localhost:8080/api/health` should return HTTP 200. This is an HTTP local demo: the listener binds `0.0.0.0`, so use a trusted local environment. Do not describe it as an HTTPS deployment. Direct HTTPS requires both certificate/key environment variables; README describes that option.

Startup creates the schema and seeds five synthetic trains only when the trains table is empty. It does not seed users or administrator credentials. If the page is missing, verify the working directory and `RTRS_WEB_DIR`; if binding fails, stop the old demo process or consistently use another port. If configure cannot download dependencies, resolve network access before the live slot; do not edit code to bypass it.

## Accounts to create in the UI

These are proposed disposable credentials, not existing or production accounts. Register each, then log in explicitly; registration does not automatically log in.

| Purpose | Full name | Email | Phone | Password |
|---|---|---|---|---|
| Booking owner | Demo Passenger | passenger@example.com | 9876543210 | DemoPass2026 |
| Second passenger | Other Passenger | other@example.com | 9876543211 | OtherPass2026 |
| Administrator | Demo Administrator | admin@example.com | 9876543212 | AdminPass2026 |

For administrator setup, first register its account. Stop the server with Ctrl+C, then run in the terminal that retains the disposable database environment:

```bash
./build/rtrs_server --promote-admin admin@example.com
./build/rtrs_server
```

Promotion uses the same `RTRS_DB`, revokes that account's sessions and requires a new login. “Manage trains” should then appear. Public registration cannot grant administrator privileges. Use independent browser profiles/private sessions for owner and second passenger, logging in explicitly in each; the token is held in `sessionStorage`. Reload currently renders the login view again, so do not rely on automatic UI session restoration.

## Journey fixture

Choose a real calendar date at least seven days after the actual demo day and write it down as **D**. Do not freeze a past date into a future presentation. Example only: if presenting on 2026-11-12, D can be 2026-11-19. Use the same D for all inventory comparisons. Schedules/cancellation are interpreted in UTC, even though the presenter may be in India.

On an untouched disposable database, Pune Jn → Mumbai CSMT has:

| Train | Departure → arrival | Duration | Relevant class / fare / initial seats |
|---|---|---|---|
| 11007 Deccan Express | 07:15 → 10:45 | 3h 30m | SL / Rs 190 / 300; CC / Rs 420 / 120; 2A / Rs 780 / 40 |
| 12027 Shatabdi Express | 17:30 → 20:40 | 3h 10m | CC / Rs 520 / 400; EC / Rs 980 / 100 |

These are project seed values in `src/db/Seed.cpp`, not real railway timetable or fare claims. Capture the actual pre-booking count if the database was already used.

## Main live sequence (approximately 6–8 minutes)

| Step | Presenter action | Expected result and explanation |
|---|---|---|
| 1. Authentication | Show owner registration or use the prepared owner login. Try one incorrect password, then the correct password. | Generic failure, then home screen. Avoid five repeated mistakes: lockout is five failures with a 300-second lock. |
| 2. Search | Search Pune Jn → Mumbai CSMT on D; leave class/fare filters empty and use departure ascending. | The two seed trains above appear with times, duration and starting fare. Station matching is case-insensitive exact matching, not autocomplete/fuzzy matching. |
| 3. Filters | Choose SL and max fare 200, then search again. | Only 11007 qualifies. Clear filters when returning to the two-result view. |
| 4. Schedule and availability | Open Deccan Express; inspect ordered stops, classes, fare and availability. Select SL. | The schedule includes Lonavala. Fresh SL availability is 300; use actual baseline A otherwise. Displayed availability is provisional. |
| 5. Validation | Enter Demo Passenger, age 0, gender Male, and preview. Correct age to 22. | Invalid age produces a field error; valid range is 1–120. A booking accepts 1–6 passengers, gender codes M/F/O. |
| 6. Preview | Preview one valid SL passenger. | Rs 190 × 1 = Rs 190. Preview alone does not reserve a seat; inventory remains A. |
| 7. Confirm | Click the generated confirmation button once. Note returned PNR and internal booking ID from the API response if needed. | A confirmed ticket has a unique PNR and assigned seat. Re-open availability: A−1 (299 for a fresh DB). The server re-checks inventory inside a transaction. |
| 8. History/ticket | Open My bookings and the newly created ticket. | Own confirmed booking appears, with passenger, seat, class, date, PNR and fare. Ticket lookup uses PNR; cancellation uses internal booking ID. |
| 9. Cancellation preview | Click Cancel ticket and inspect the quote; optionally choose the offered back action before trying again. | With D seven days away, eligible under the explicit 24h/90% settings. Quote is Rs 171, without changing booking/inventory until confirmation. |
| 10. Cancel | Confirm cancellation; return to history and re-open availability. | Status CANCELLED; cancellation timestamp and recorded Rs 171 refund; inventory restored to A. This records a refund and transfers no money. Repeat cancellation cannot restore seats again. |
| 11. Logout | Log out and attempt a protected request using that session if showing API evidence. | Logout deletes the server-side session; protected requests with it receive 401. Returning to the login page alone is not proof of invalidation. |

## Additional evaluator scenarios

Use these after the main flow, time permitting. They are expected outcomes, not completed tests.

| Scenario | Procedure | Expected result |
|---|---|---|
| Ownership | Log in as the second passenger in an independent browser context. Request the owner's ticket PNR or cancellation URL through DevTools/API using the second account's bearer token. | Owner's booking is absent from this user's history. Ticket, cancellation quote and cancel return 404 for another owner's booking, just as for an unknown booking. Empty history alone is not full authorization evidence. |
| Duplicate cancellation | As owner, repeat cancellation of the cancelled internal ID. | HTTP 409; no second restoration or refund event. UI may no longer offer the button, so use API evidence if needed. |
| Cutoff refusal | Create a separate one-passenger booking for today's UTC date on 11007; preview cancellation under 24h cutoff. | Current code accepts calendar-valid booking dates without a future-departure restriction. Today's origin departure is less than 24h away or already past: quote says ineligible; confirm returns 409; inventory unchanged by refusal. Disclose this date-validation limitation. |
| Profile | Change owner's name/phone; inspect disabled email. Optionally change password last. | Name/phone update, email immutable. Password change keeps the calling session and revokes other sessions. Record changed demo credentials or reset afterward. |
| Admin access | Login as prepared admin; open Manage trains. Try `GET /api/admin/me` with an ordinary user's bearer token. | Admin can manage trains; passenger request gets 403 (no token gets 401). Role is checked from current database state. |
| Admin create/edit | Add number 99001, name Demo Express; origin Pune Jn departure 12:00/day 0, destination Mumbai CSMT arrival 16:00/day 0; blank origin arrival/destination departure. Class SL, Sleeper, fare 200, seats 2. Edit fare to 210. | Creation and edit succeed atomically. Search displays the added service. Duplicate number is 409. Do this after the two-result search demonstration. |
| Admin deactivate | Deactivate only Demo Express; search again. | Disappears from passenger search; new booking/stale preview rejected. Existing tickets and eligible cancellation remain accessible. UI edit does not reactivate it. |
| Audit | As admin call `GET /api/admin/audit?limit=10` with bearer header in DevTools/API. | Newest events first, with booking/cancellation identifiers and refund, without passwords, tokens or passenger personal details. There is no need to invent an audit UI. |

API paths: `GET /api/bookings/{PNR}` reads a ticket; `GET /api/bookings/{internalId}/cancellation` quotes; `PUT` or `POST /api/bookings/{internalId}/cancel` confirms. Supply `Authorization: Bearer <the current context's token>`. Obtain synthetic-session values from Network responses; do not put tokens in URLs or screenshots. Cancellation eligibility quote returns HTTP 200 even when `eligible` is false.

Concurrency is better explained using `tests/test_concurrency.cpp` than improvised by clicking twice. Its defined scenario is eight buyers competing for two seats, expecting two 201 responses, six 409 responses and distinct seats. Cite an actual test run only when you have one.

## Reset and recovery

For a full clean reset, stop the server with Ctrl+C and leave the old disposable directory intact. Run the setup environment block again to allocate a new database directory, then register accounts and promote admin again. No real database needs deletion. The normal `rtrs.db` is untouched when the documented `RTRS_DB` is used. Clear site session storage/close demo contexts and log in again; browser credentials cannot authenticate against the fresh database.

For a same-data restart, stop and restart `./build/rtrs_server` with the same exported variables: accounts/bookings should persist in SQLite. This is a manual persistence check to perform, not proof of crash recovery. Do not copy a live SQLite database without accounting for its WAL; do not reset by manipulating individual reservation rows.

If cancellation fails unexpectedly, check D, UTC origin departure, policy variables and current train schedule. If inventory differs from this guide, use the measured baseline or start a fresh disposable database. If booking fails, refresh availability and report the actual error. Do not present a failed rehearsal as successful.

## Evidence to collect during the real rehearsal

Record operator, actual date/time/timezone, revision, environment/policy values, CTest result, scenario IDs/outcomes and any defects. Capture only synthetic UI states and redact tokens. Leave outcomes unrecorded until performed. The linked reports provide current measured coverage and selected automated/browser results; this document provides no human acceptance approval or signature. See [DEVIATIONS.md](../DEVIATIONS.md) for limits and [VIVA_PREP.md](VIVA_PREP.md) for implementation answers.
