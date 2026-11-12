# Video handoff to the friend

Updated 2026-11-12. This is a recording plan only; the friend owns capture, narration, editing and export. No video is created or claimed complete here. Both videos will be **recorded now to demonstrate historical increments**, not presented as archive footage shot during the course windows. Show the actual recording date and demonstrated source SHA on a title/end card.

## Verified milestone mapping

The course windows come from [BACKLOG.md](../planning/BACKLOG.md), reconstructed from repository evidence; they are not proof of original Jira sprint membership. Full SHAs below were verified with read-only `git log`.

| Video / milestone | Course window | Scope to demonstrate | Historical source endpoint |
|---|---|---|---|
| Video 1: 60s / Sprint 1 | 19–23 October 2026 | Existing authentication foundation, search, booking and history through RTRS-24 | `f54ab03a8bfcb680b4cf7092b0b236e35bb6ad0d`, 2026-10-20T23:22:24+05:30 |
| Video 2: 120s / Sprint 2 | 26–30 October 2026 | Sprint 1 flow plus cancellation, refund quote and seat restoration through RTRS-28 | `7c5abfcc993c7e1c74c2fd4b1aea80be86fc869c`, 2026-10-29T11:24:22+05:30 |
| Optional extra / Sprint 3 | 2–6 November 2026 | Admin, audit and security/NFR increment | `6b5464298c949806ebba0b7bd1d903c962b5255d`, 2026-11-03T18:24:41+05:30 |

Authentication was an earlier foundation mapped into Sprint 1; do not redate its October 13 implementation. Cancellation arrived October 29 and must not appear in Video 1. Admin/security arrived November 3 and is an optional Sprint 3 extra, not required in Video 2. Commit endpoints identify source increments rather than asserting every associated PR merged by that endpoint. Current final-build tests do not retroactively validate these earlier source snapshots.

## Preparation and version choice

Use [LIVE_DEMO_RUNBOOK.md](LIVE_DEMO_RUNBOOK.md) for synthetic passenger details, seed train values and the main procedure, with the snapshot differences below. Choose date D at least seven days after the actual recording day. Book one SL passenger on 11007 Deccan Express, Pune Jn → Mumbai CSMT: Rs 190 and initial availability A=300 on a fresh database. Video 2 cancellation should quote Rs 171 under provisional 24h/90% defaults and restore A after confirmation. No bank/payment transfer occurs.

For faithful source reproduction, use the historical snapshots below and explicitly label their SHAs. Alternatively, capture only the relevant subset of the final build, labelled “current build demonstrating historical scope”; this is not proof that the old source executes identically. Do not show admin/cancellation controls from a newer build as if they existed in Sprint 1. Rehearse the actual version chosen; old builds can fail with today's compiler/dependency environment. No historical build or rehearsal was executed for this handoff, and no success is guaranteed.

### Optional detached snapshots — instructions only

Run from the main repository root. These commands are supplied for the friend/operator; **they have not been executed here**. They create separate detached worktrees without switching or resetting the current checkout. Each has its own build and disposable DB. Initial dependency downloads require internet; CMake 3.20+, C/C++17 tools and OpenSSL development files are needed.

```bash
export RTRS_VIDEO_ROOT="$(mktemp -d /tmp/rtrs-video.XXXXXX)"
git worktree add --detach "$RTRS_VIDEO_ROOT/sprint1" f54ab03a8bfcb680b4cf7092b0b236e35bb6ad0d
git worktree add --detach "$RTRS_VIDEO_ROOT/sprint2" 7c5abfcc993c7e1c74c2fd4b1aea80be86fc869c
cmake -S "$RTRS_VIDEO_ROOT/sprint1" -B "$RTRS_VIDEO_ROOT/sprint1/build" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$RTRS_VIDEO_ROOT/sprint1/build" -j2
ctest --test-dir "$RTRS_VIDEO_ROOT/sprint1/build" --output-on-failure
cmake -S "$RTRS_VIDEO_ROOT/sprint2" -B "$RTRS_VIDEO_ROOT/sprint2/build" -DCMAKE_BUILD_TYPE=Debug
cmake --build "$RTRS_VIDEO_ROOT/sprint2/build" -j2
ctest --test-dir "$RTRS_VIDEO_ROOT/sprint2/build" --output-on-failure
```

Run one server at a time (Ctrl+C before switching). Absolute web/DB paths avoid accidentally sharing the final checkout's database or UI:

```bash
RTRS_DB="$RTRS_VIDEO_ROOT/sprint1.sqlite" RTRS_WEB_DIR="$RTRS_VIDEO_ROOT/sprint1/web" RTRS_PORT=8081 "$RTRS_VIDEO_ROOT/sprint1/build/rtrs_server"
```

Open `http://localhost:8081`, register a disposable passenger and log in. For Sprint 2, use:

```bash
RTRS_DB="$RTRS_VIDEO_ROOT/sprint2.sqlite" RTRS_WEB_DIR="$RTRS_VIDEO_ROOT/sprint2/web" RTRS_PORT=8082 RTRS_CANCEL_CUTOFF_HOURS=24 RTRS_REFUND_PERCENT=90 "$RTRS_VIDEO_ROOT/sprint2/build/rtrs_server"
```

Open `http://localhost:8082`; create its passenger independently. Inspected historical `main.cpp` versions serve HTTP and lack the later admin-promotion/TLS CLI and production PBKDF2 environment configuration; do not apply those final-runbook setup steps to them. Sprint 1 has no cancellation feature. These snapshots seed project data, not real timetables. Calendar-valid future D avoids stale hardcoded recording dates; it does not guarantee every old test fixture is time-independent. If a historical build/test fails, preserve/report the actual output; do not silently patch it or claim final-build evidence covers it. For a retake use a fresh disposable DB path after stopping the server. Leave temporary worktrees intact until capture is reviewed; no forced deletion/reset is required.

## Video 1 — 60 seconds: Sprint 1 only

| Time | Genuine capture / action | Suggested narration |
|---|---|---|
| 0–7s | Title: recorded on actual date; Sprint 1 window and chosen SHA/build mode | “Recorded now to demonstrate the Sprint 1 authentication, search and booking increment.” |
| 7–16s | Register passenger or show prepared login | “A passenger registers and logs in to an authenticated session.” |
| 16–28s | Search future D, Pune Jn → Mumbai CSMT; open Deccan Express schedule/SL availability | “Search displays project train schedules, class fares and current inventory.” |
| 28–40s | Enter passenger and preview Rs 190 | “Preview validates details and fare without reserving a seat.” |
| 40–51s | Confirm, ticket/PNR/seat, inventory A−1 | “Confirmation assigns a unique PNR and seat and reduces inventory by one.” |
| 51–60s | Own booking history and logout; end card | “The confirmed booking is available in the passenger's history. This is an academic prototype.” |

No cancellation/refund footage or admin operation belongs in this cut. Cuts may remove waiting but must preserve consistent genuine ticket and inventory states.

## Video 2 — 120 seconds: Sprint 2 adds cancellation

| Time | Genuine capture / action | Suggested narration |
|---|---|---|
| 0–10s | Title: actual recording date, Sprint 2 window and chosen SHA/build mode | “Recorded now to demonstrate the increment completed through October 29: cancellation, refund recording and restoration.” |
| 10–25s | Passenger login and future-date route search | “The earlier authentication and search foundation remains available.” |
| 25–42s | Deccan Express SL schedule/fare/availability A | “One SL passenger costs 190 rupees on this project seed service.” |
| 42–58s | Passenger details and preview | “Preview does not hold inventory.” |
| 58–76s | Confirm → ticket/PNR → own history and inventory A−1 | “Confirmation creates the booking and assigns a seat.” |
| 76–93s | Cancel ticket → eligible quote Rs 171; hold readable quote | “Provisional settings allow cancellation until 24 hours before UTC origin departure and calculate a 90-percent recorded refund.” |
| 93–110s | Confirm cancellation → CANCELLED/time/refund; re-open inventory A | “Confirmation records 171 rupees and restores the seat once. No payment provider transfers money.” |
| 110–120s | History/logout and limits card | “Cancelled history remains. Human acceptance and policy approval are still pending.” |

Admin, audit and later security work may be a separately labelled Sprint 3 extra using the November 3 source; they are not required footage for Video 2. Do not imply the later TLS, performance or security improvements were already delivered in Sprint 2.

## Current final-build evidence and attribution

On 2026-11-12, the final November 3 build passed [201 tests in 49 suites](../reports/Unit_Test_Report.md), with [94.7% line](../reports/Code_Coverage_Report.md), [46.7% branch](../reports/Branch_Coverage_Report.md) and 100% function coverage in the measured scope excluding main.cpp. [122 system checks](../reports/System_Validation_Report.md) passed, including real TLS 1.3, restart/forced-stop post-commit persistence and integrity, and small-load search/booking p90 0.002256s/0.006130s. [Selected desktop/mobile flows](../reports/Browser_Validation_Report.md) also passed. These are current final-build results, not historical snapshot results, archive footage or human acceptance.

Roles/USNs: Deeptanshu Kumar, developer, PES1UG24AM357; Amar, tester, PES1UG24AM340; Aaryaprasad, PO, PES1UG24AM331 (SRS spelling Aryaprasad). Use [VIVA_PREP.md](VIVA_PREP.md) for authentic contribution anchors. Do not infer equal code authorship from assigned roles or label merged PRs as proof of manual test execution.

## Friend's capture/export checklist

- Use synthetic accounts; hide tokens, auth headers and unrelated personal data; mute notifications.
- Keep D, PNR, fare and inventory consistent across shots from the same snapshot/database. Actual displayed state takes precedence over an expected overlay.
- Show 24h/90% as provisional and say “recorded refund”, never “money received”. Policy approval/signatures and human UAT remain pending.
- Keep title cards explicit about actual recording date, historical course window and whether footage uses an old snapshot or current-build subset.
- Do not claim production readiness, all browsers/devices, power-loss recovery, or 100% requirements/branch coverage. No real railway integration is shown.
- Export readable 60s and 120s cuts; actual review, approvals and video completion must be recorded only after they occur. No video was produced by this documentation task.
