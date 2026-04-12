## Why

Current inbox behavior preserves a startup backlog and deferred retry states that are not product requirements. A line can be read from file, held in memory, retried later, or dropped on file reset. The inbox should instead be a live runtime transport that accepts commands only when ConsoleLink is ready.

## What Changes

- Retire the combined `command-submission` capability and replace it with separate `inbox` and `console-commands` capabilities.
- Treat the inbox as a session-scoped runtime transport instead of a startup backlog.
- Rename the config surface from command-submission terminology to inbox terminology.
- Clear pre-ready inbox content in one physical inbox reset when the session baseline is established, announce the reset via `[local]` only after the inbox is known empty, and fail fast if startup cannot establish that empty baseline.
- Treat later inbox file rewrites during runtime as new live input rather than as stale startup backlog.
- Announce local-command readiness after `kDataLoaded`.
- Announce when console commands become ready and when they become unavailable because the console is closed.
- Accept only lines appended after the current session baseline.
- Route `cdbg.*` lines to local-command handling and let the local-command processor decide readiness.
- Route non-local lines to console-command handling and let the console-command processor decide readiness.
- Split implementation ownership into separate folders for inbox transport, local-command handling, and console-command handling.
- Remove deferred and retry queue semantics that only existed to preserve startup backlog behavior.

## Capabilities

### New Capabilities
- `inbox`: Session-scoped inbox transport and routing policy.
- `console-commands`: Console-command readiness and engine submission through the validated seam.

### Modified Capabilities
- `local-commands`: Local-command behavior starts only after inbox routing admits the line.
- `plugin-settings`: Inbox config uses inbox terminology instead of command-submission terminology.
