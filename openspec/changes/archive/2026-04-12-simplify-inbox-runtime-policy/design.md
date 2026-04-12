## Context

This change replaces one mixed submission service with three owned boundaries. The technical goal is to keep inbox transport, local-command execution, and engine-backed console-command execution in separate modules with explicit handoff contracts.

## Decisions

1. `InputProcessor` owns transport and routing only.
Owner: `src/inbox/`.
Responsibilities: watch the inbox file, perform the one-time session-start clear, establish the post-clear baseline only after the inbox file is known empty, fail fast if startup cannot establish that empty baseline, treat later runtime file rewrites as new live input instead of replay-protected startup backlog, extract appended newline-terminated lines, classify each line as local or console, route each line to exactly one execution processor, and emit the `[local]` session-clear notice only after baseline establishment succeeds.
Boundary: `InputProcessor` does not parse `cdbg.*` arguments and does not call the engine submission seam directly.

2. `LocalCommandProcessor` owns admitted local-command execution.
Owner: `src/local_commands/`.
Responsibilities: accept already-routed `cdbg.*` lines, decide whether local-command readiness is active, announce local-command readiness, reject pre-ready local commands through `[local]`, parse command name and arguments, execute supported local commands, produce local response lines, and return a terminal outcome to the caller.
Boundary: inbox mutation stays outside this processor.

3. `ConsoleCommandProcessor` owns admitted engine submission.
Owner: `src/console_commands/`.
Responsibilities: accept already-routed non-local lines, decide whether console-command readiness is active, reject not-ready console commands through `[local]`, track whether the validated `ExecuteCommand` seam is available, track whether the console is open, submit admitted non-local lines through that seam, record `[console-submit]` only after real submission, announce readiness transitions through `[local]`, and stop later submission attempts after terminal seam failure.
Boundary: inbox file lifecycle and local-command routing stay outside this processor.

4. Processor handoff is one-way and explicit.
`InputProcessor` hands a line to exactly one execution processor after classification. `LocalCommandProcessor` and `ConsoleCommandProcessor` do not read the inbox themselves and do not route work between each other. Shared helpers may exist, but no mixed transport-and-execution module remains the canonical owner.

5. Activity ownership follows processor ownership.
`InputProcessor` owns the `[local]` session-clear line. `LocalCommandProcessor` owns `[local]` readiness, rejection, request, and response activity for local commands. `ConsoleCommandProcessor` owns `[local]` readiness and rejection activity for console commands, plus `[console-submit]` for commands that actually traverse the engine seam.

6. `console-submit` stays narrow.
Only commands that actually traverse the engine submission seam are recorded as `[console-submit]`. Session baseline, readiness, and rejection notices stay on `[local]`.
