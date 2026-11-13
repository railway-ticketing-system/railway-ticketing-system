# Browser Validation Report

Executed on 12 November 2026 against the freshly built C++ server, using an isolated synthetic SQLite database and Chrome browser context. Application commit `6b5464298c949806ebba0b7bd1d903c962b5255d`.

| Viewport | Actual exercised screens | Result |
|---|---|---|
| Desktop 1366 × 900 | Registration, login, search Pune Jn → Mumbai CSMT, two matching results | Passed; document width fits viewport |
| Mobile 390 × 844 | Login, search, schedule, availability, booking form and preview, confirmed ticket, cancellation quote, cancelled ticket | Passed; document width remains 390px throughout inspected states |

The UI booked one SL passenger on train 11007 for 2026-11-15 at Rs 190. The confirmed ticket assigned SL-1 and PNR 6690010819. The cancellation quote showed 90% refund, and confirmation rendered CANCELLED with recorded refund Rs 171. These are disposable test records, not real railway tickets.

## Screenshots

- [Desktop search](../evidence/ui-desktop-search.png)
- [Mobile search](../evidence/ui-mobile-search.png)
- [Mobile booking preview](../evidence/ui-mobile-booking.png)
- [Mobile confirmed ticket](../evidence/ui-mobile-ticket.png)
- [Mobile cancelled ticket](../evidence/ui-mobile-cancelled.png)
- [Structured observation record](../evidence/browser-validation.json)

## Reproduction and limits

Start the server with a new disposable RTRS_DB, open a Chrome context, register/login, choose a future date beyond the cancellation cutoff, then follow the live demo runbook. Set the two viewport sizes above and inspect both document width and complete user controls. This is evidence for selected RTS-NF-007 / TC-UX-01 scenarios. It is not a physical-device test, full accessibility audit, human UAT acceptance, or proof of every viewport/browser combination. Desktop booking was not independently repeated in this run. API ownership and persistence checks are recorded separately in the System Validation Report.
