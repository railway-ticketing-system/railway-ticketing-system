# railway-ticketing-system
A railway ticket reservation system with booking and cancellation features.

Product Owner - Aaryaprasad

Developer - Deeptanshu Kumar

Testing - Amar

## Tech stack
- **Backend:** C++17 REST API ([cpp-httplib](https://github.com/yhirose/cpp-httplib), [nlohmann/json](https://github.com/nlohmann/json)), SQLite
- **Security:** PBKDF2-HMAC-SHA256 password hashing and random session tokens (OpenSSL), parameterised SQL only
- **Frontend:** static HTML/CSS/JavaScript served by the backend (`web/`)
- **Tests:** GoogleTest, gcov/gcovr coverage (line + branch)
- **CI/CD:** GitHub Actions (build, test, coverage, cppcheck, CodeQL, Docker build and GHCR publish)

## Build and run
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build -j
ctest --test-dir build --output-on-failure
./build/rtrs_server          # http://localhost:8080
```
Environment variables: `RTRS_PORT` (default 8080), `RTRS_DB` (default `rtrs.db`), `RTRS_WEB_DIR` (default `web`).

### Cancellation and refund policy
- `RTRS_CANCEL_CUTOFF_HOURS` defaults to **24**: confirmed tickets can be cancelled until 24 hours before the train's first scheduled departure (including its day offset). Schedule dates/times are interpreted as UTC.
- `RTRS_REFUND_PERCENT` defaults to **90**: the refund is this percentage of the stored total booking fare, rounded down to whole rupees. These configurable defaults are project policy, pending the team's final policy decision.
- The ticket screen shows eligibility and the refund before cancellation is confirmed. Cancellation records the timestamp and refund amount; this is a recorded refund, with no payment-provider transfer.
- `GET /api/bookings/{id}/cancellation` previews eligibility and refund; `PUT /api/bookings/{id}/cancel` (also supported as `POST`) confirms cancellation and returns the updated ticket. Both actions require the booking owner's active session. Unknown bookings and other passengers' bookings return the same 404 response.

Docker: `docker build -t rtrs . && docker run -p 8080:8080 rtrs`

### Performance
Search, availability, and booking inventory lookups use dedicated SQLite indexes (case-insensitive station lookup, train classes, seat reservations, and bookings by train/date/class/status). The `PerformanceApi.P90SearchLatencyStaysUnderThreeSeconds` test exercises concurrent search requests and asserts the 90th percentile stays below the 3-second requirement (RTS-NF-001 / TC-PERF-01).

Seat allocation runs in `BEGIN IMMEDIATE` transactions with a busy timeout, so competing writers wait and re-check inventory atomically. A database trigger additionally rejects any duplicate seat for the same train, date, and class. `ConcurrencyApi.ConcurrentBookingsAllocateEachSeatOnlyOnce` verifies that simultaneous confirmations never double-allocate a seat (RTS-NF-002 / TC-CONC-01).

## Workflow
### Administrator access
Register an ordinary account first, then grant it the admin role from the server host:
```bash
RTRS_DB=rtrs.db ./build/rtrs_server --promote-admin admin@example.com
```
This local command revokes the account's existing sessions. Log in again with its existing credentials; the home screen will show **Manage trains**. Public registration always creates a passenger, regardless of any submitted role. Admin endpoints recheck the current role on every request; there are no default admin credentials.

The admin screen lists train records and provides a form for adding a train with its ordered stops and travel classes. Admin-only APIs:
- `GET /api/admin/me` checks administrator access.
- `GET /api/admin/trains` lists full records; `GET /api/admin/trains/{id}` reads one.
- `POST /api/admin/trains` creates a train, stops, fares, and seat capacities in one transaction.
- `PUT /api/admin/trains/{id}` replaces a train's details, ordered stops, and classes atomically. Existing booking fares and reservations are preserved. Referenced classes cannot be removed, and capacity cannot be reduced below reserved quantities or existing assigned seat numbers; these conflicts return 409.
- `PUT /api/admin/trains/{id}/deactivate` deactivates the whole service and its schedule without deleting records. Inactive trains disappear from passenger search, schedule, fare, and availability endpoints and reject new bookings (including stale previews). Existing tickets, history, and eligible cancellations remain available. Repeating deactivation is safe; editing an inactive record does not reactivate it.
- `GET /api/admin/audit?limit=100` returns the newest audit events first (`limit` 1-500, default 100).

Booking and cancellation events are written to the append-only `audit_events` table in the same database transaction as the reservation change. Each event stores the timestamp, event type, user, booking, train, PNR, and cancellation refund amount. Audit records never contain passwords, session tokens, or passenger personal details.

Train payloads contain `number` (five-digit string), `name`, `stops`, and `classes`. Each stop has `station`, `arrival`, `departure` (HH:MM), and integer `dayOffset` (0-30). Leave the origin arrival and destination departure blank. Stops must be chronological; increase the day offset for overnight journeys. Each class has `code` (`SL`, `3A`, `2A`, `1A`, `CC`, `EC`), `name`, integer `fare` (1-1000000 rupees), and integer `totalSeats` (1-10000). Validation errors are indexed by row, such as `stops.1.arrival`; duplicate train numbers return 409.

1. Developer works on one feature branch per Jira story (e.g. `RTRS-9-user-registration`), one commit per story.
2. Pull request into `test`; CI must be green.
3. Tester verifies on `test`, then merges `test` into `main`.
4. Merge to `main` publishes the Docker image (CD).

Commit messages start with the Jira key, e.g. `RTRS-9: Add user registration`.
