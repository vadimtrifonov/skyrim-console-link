## 1. Implemented

- [x] 1.1 Rename `ConsoleActivityKind::kLocalCommandActivity` to `kLocalActivity` and update the affected taxonomy, emitters, docs, and tests.
- [x] 1.2 Rename the structured activity log artifact to `ConsoleLink.activity.log` and update affected diagnostics and references.
- [x] 1.3 Remove print end to end, including direct print, outbox support, related settings, and print-only runtime wiring.

## 2. Verified

- [x] 2.1 Add or update automated coverage for the local activity taxonomy, the activity log filename, and the configuration behavior after print removal.
- [x] 2.2 Run the relevant automated test target and record the result.

Verification result: `xmake` succeeded and `xmake run ConsoleLinkTests` passed on 2026-04-12.

## 3. Deferred

- None.
