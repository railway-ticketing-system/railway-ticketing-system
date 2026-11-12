# Final submission checklist

Prepared on **12 November 2026** for Team 15, Railway Ticket Reservation System, UE24CS341A. This index maps the five course handouts to concrete repository artifacts. Application feature baseline: `6b5464298c949806ebba0b7bd1d903c962b5255d`; tester's main integration: `fb1b4d55957ab8022e136396d656bec870c08375` (PR 12). Current reports are fresh verification, with actual run dates.

## Documents and diagrams

| # | Deliverable | Artifact | Status / remaining action |
|---|---|---|---|
| 1 | Updated IEEE-style SRS | [PDF](Railway_Ticket_Reservation_System_SRS.pdf), [editable Word](Railway_Ticket_Reservation_System_SRS.docx) | Prepared for team review; original baseline remains in Git history |
| 2 | At least one UML use-case diagram | SRS and [diagram](diagrams/use_cases.png) | Included |
| 3 | At least two security objectives and requirements | SRS security section; five RTS-SR requirements | Included |
| 4 | Updated IEEE-style Test Plan; sections 3, 4, 5 and security validation 5.1 | [Word](Railway_Ticket_Reservation_System_Test_Plan_Final.docx), [PDF](Railway_Ticket_Reservation_System_Test_Plan_Final.pdf) | Prepared for QA review |
| 5 | At least 7–10 test cases updated after sprint work | Test Plan detailed cases and [machine-readable cases](source/test_cases.json) | Detailed inputs, steps, expected results, requirement and test mappings included |
| 6 | Updated IEEE-style architecture/design | [Word](Railway_Ticket_Reservation_System_SAD_Final.docx), [PDF](Railway_Ticket_Reservation_System_SAD_Final.pdf) | Prepared for review; describes actual C++ stack |
| 7 | Two UML sequence diagrams, API design and error handling | SAD; [booking sequence](diagrams/booking_sequence.png), [cancellation sequence](diagrams/cancellation_sequence.png), [component diagram](diagrams/components.png) | Included |

## Planning and implementation evidence

| # | Deliverable | Artifact | Status / remaining action |
|---|---|---|---|
| 8 | GitHub Project linked to repository | [Board import script](../scripts/submission/import_github_project.py) | **Blocked by missing GitHub Projects scope**; repository access alone cannot create the visual board |
| 9 | FR/NFR backlog, estimates and member assignments | [Backlog](planning/BACKLOG.md), [data](planning/backlog.json), [published issue manifest](evidence/github-backlog-import.json) | Repository-backed stories published with owner and course milestone. Points are explicitly retrospective estimates pending original Jira reconciliation |
| 10 | Sprint 1 and 2 plans | [Course sprint mapping](planning/BACKLOG.md), repository milestones | Course dates stored as scheduled windows. Original six-sprint membership is preserved in the recovered 13 October snapshot; later live dates/statuses require connection |
| 11 | GitHub Actions automation | [CI](../.github/workflows/ci.yml), [CodeQL](../.github/workflows/codeql.yml), [CD](../.github/workflows/cd.yml), [run evidence](evidence/ci_runs.json) | Build/test/coverage/static checks/Docker; CD publishes image on main. New system validation and raw result artifacts added to CI |
| 12 | Individual PR and review evidence | [PR records](evidence/pull_requests.json), [commits](evidence/commits.tsv), [formal review records](evidence/formal-pr-reviews.json), [viva attribution](demo/VIVA_PREP.md) | Genuine developer/tester authorship and merges preserved. Formal submitted reviews are not fabricated; existing merges do not prove an independent review |
| 13 | Sprint 1 working increment | Existing authentication/search/booking implementation; historical PRs linked in backlog | Implemented; course Sprint 1 builds on earlier dry-run authentication work |
| 14 | Sprint 1 1-minute video | [Friend's video handoff](demo/VIDEO_HANDOFF.md), [upload location](videos/README.md) | **Friend to record/upload** |
| 15 | Sprint 2 working increment, finalized documentation and freeze | Existing cancellation implementation; [freeze record](planning/FEATURE_FREEZE.md) | Sprint 2 implementation delivered; full application uses permitted Sprint 3. Freeze record entered now against the existing final feature baseline |
| 16 | Sprint 2 2-minute video | [Friend's video handoff](demo/VIDEO_HANDOFF.md), [upload location](videos/README.md) | **Friend to record/upload** |
| 17 | Explained SRS/SAD/Test Plan deviations | [Deviation record](DEVIATIONS.md) and revised official documents | Actual stack, interfaces, policy and verification boundaries documented; PO/mentor acceptance remains a human action |
| 22 | Optional Sprint 3 if spillover | Course Sprint 3, 2–6 Nov; [backlog](planning/BACKLOG.md) | Existing admin/audit/security changes delivered 3 Nov; original Jira membership not assumed |

## Testing reports in docs

| # | Deliverable | Submission report | Evidence / result |
|---|---|---|---|
| 18 | Unit Test Report | [PDF](reports/Unit_Test_Report.pdf), [Markdown](reports/Unit_Test_Report.md) | **201/201 passing**, 49 suites; [XML](reports/unit-tests.xml) |
| 19 | System Validation Report | [PDF](reports/System_Validation_Report.pdf), [Markdown](reports/System_Validation_Report.md) | **122/122 checks passing**; [raw JSON](reports/system_validation.json); real HTTP/TLS, restart recovery, access control and complete booking/cancellation workflows |
| 20 | Code Coverage Report | [PDF](reports/Code_Coverage_Report.pdf), [Markdown](reports/Code_Coverage_Report.md), [source HTML](reports/coverage.html) | **94.7% lines, 1476/1558**; C++ src only, excludes main.cpp |
| 21 | Branch Coverage Report | [PDF](reports/Branch_Coverage_Report.pdf), [Markdown](reports/Branch_Coverage_Report.md) | **46.7% branches, 3162/6778**; includes compiler-generated C++ exception/cleanup branches |
| — | Responsive UI acceptance evidence | [Browser report](reports/Browser_Validation_Report.md), [screenshots](evidence/README.md) | Selected desktop 1366×900 and mobile 390×844 scenarios; not a cross-browser/accessibility certification |

Local system performance: search p90 **2.256 ms**, booking p90 **6.130 ms**, each 20 requests with four concurrent clients against small seeded local data. These are not internet latency or production capacity claims. TLS verification uses a temporary test trust anchor; restart checks cover previously committed state, not power-loss durability or every database outage.

## Demo and individual evaluation

| # | Deliverable | Artifact | Status / remaining action |
|---|---|---|---|
| 23 | Final live demo | [Runbook](demo/LIVE_DEMO_RUNBOOK.md) | Setup, synthetic fixtures, workflows, negative cases and recovery ready; team must present live |
| 24 | Individual viva | [Preparation guide](demo/VIVA_PREP.md) | Implementation Q&A and authentic individual evidence prepared; each member must explain their work |

## Actions requiring people or account access

1. Connect the Atlassian integration, or provide a current Jira export/screenshots, to reconcile the recovered [13 October snapshot](evidence/jira-snapshot-2026-10-13.json) with later sprint dates and status changes. The snapshot preserves descriptions and six-sprint membership but returned no story-point fields. Do not paste an API token into chat.
2. Authorize the GitHub CLI `project` scope to create the visual Project; then run the idempotent board importer. The published issues/milestones are available independently.
3. Friend records/uploads the two videos. Amar reviews the submission PR, performs human acceptance and merges the tested final package into main.
4. Aaryaprasad reviews the architecture/policy deviations and records genuine acceptance; mentor/team approval fields remain unsigned until actual approval.

No commits, reviews, approvals or creation timestamps have been backdated. Scheduled course date fields and retrospective records are labelled separately from original activity.

## Reproduce and update

- Install reporting dependencies in a virtual environment using `scripts/submission/requirements.txt`; build prerequisites are CMake, GCC, OpenSSL development headers, Python and openssl CLI.
- Run `python3 scripts/submission/collect_reports.py --build-dir build-submission --jobs 4` for a new genuine test/coverage/system run. Regenerated reports carry new actual timestamps.
- Render printable reports with `python3 scripts/submission/render_report_pdfs.py`.
- Update the official document sources and run `scripts/submission/build_documents.py` as described by that script.
- Board publication scripts are idempotent; they require authenticated GitHub scopes and retain true creation/closure timestamps.

## Course sources

Part 1 (p. 1): requirements/design/test-plan content and initial deadline 28 Sept 2026. Part 2 (p. 1): backlog deadline 12 Oct, dry run 12–16 Oct, Sprint 1 19–23 Oct, Sprint 2 26–30 Oct and videos. Part 3 (p. 1): testing reports and optional Sprint 3, 2–6 Nov. Checklist (pp. 1–3): repository evidence and final demo/viva. FAQs (p. 1): final artifact availability, deviations and individual contribution assessment.
