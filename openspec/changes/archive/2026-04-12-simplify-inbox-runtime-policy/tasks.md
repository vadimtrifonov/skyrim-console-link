## 1. Implemented

- [x] 1.1 Split the mixed command submission path into `src/inbox/`, `src/local_commands/`, and `src/console_commands/` ownership boundaries with explicit processor handoff.
- [x] 1.2 Implement the session-start inbox clear, post-baseline routing, and processor-owned readiness/rejection behavior for local and console commands.
- [x] 1.3 Retire the old mixed command-submission implementation surface and update runtime wiring to the new processor structure.
- [x] 1.4 Rename the config terminology from command-submission terms to inbox terms.

## 2. Verified

- [x] 2.1 Add or update automated coverage for inbox session clear/routing, local-command readiness rejection and readiness announcement, and console-command readiness rejection and readiness transitions.
- [x] 2.2 Run the relevant automated test target and confirm the change passes.
Evidence: `powershell -ExecutionPolicy Bypass -File scripts/Test.ps1 -Mode debug` passed on 2026-04-11.

## 3. Deferred

No intentional carry-forward items.
