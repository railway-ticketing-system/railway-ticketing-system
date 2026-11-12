# Evidence inventory

This folder contains genuine repository metadata and synthetic browser observations collected on 12 November 2026. JSON timestamps and Git dates remain unchanged.

| Evidence | Purpose |
|---|---|
| `commits.tsv` | Commit SHA, author date, author and subject from the existing Git history |
| `pull_requests.json` | PR authors, bases, merge times and URLs |
| `ci_runs.json` | GitHub Actions conclusions and links; a passing CD run means image publication, not hosted deployment |
| `contributors.json` | GitHub contributor totals at collection time; totals include merges/documents and are not feature-completion metrics |
| `pr-7.json`, `pr-12.json` | Tester merge evidence for the initial and final feature deliveries |
| `pr-12-reviews.json` | Formal review objects for PR 12; an empty list does not demonstrate a formal submitted review |
| `issues.json` | Pre-import GitHub issue snapshot; Jira items are not implied by this file |
| `browser-validation.json`, `ui-*.png` | Genuine browser observations and screenshots from disposable demo data |

See `../reports/` for current unit XML, coverage JSON/HTML, system validation and readable reports. GitHub Project import evidence is created only after the account grants the Projects scope. `jira-snapshot-2026-10-13.json` preserves the recovered historical RTRS descriptions/sprint metadata, filtered to this project with account identifiers omitted. Live/current Jira history/screenshots still require connection or a current export.
