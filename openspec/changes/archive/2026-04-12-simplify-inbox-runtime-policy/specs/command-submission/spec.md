## REMOVED Requirements

### Requirement: Plugin can submit file-backed console commands on Skyrim VR
**Reason**: The old capability mixed inbox transport with engine submission. Those concerns now have separate canonical homes.
**Migration**: Use `inbox` for session baseline and routing. Use `local-commands` and `console-commands` for readiness-gated execution. Use `console-commands` for `[console-submit]`.

### Requirement: Command submission starts only on the validated submission path
**Reason**: Validated-path readiness is now part of `console-commands`, not the retired combined capability.
**Migration**: Use `console-commands` for readiness and submission-path rules.

### Requirement: Command submission defers safely until the console UI is available
**Reason**: The inbox is no longer a retry queue.
**Migration**: Use `console-commands` for readiness state and not-ready rejection behavior.

### Requirement: Command submission resets stale queued work when the inbox file changes identity
**Reason**: Inbox file lifecycle now belongs to the `inbox` capability.
**Migration**: Use `inbox` for session baseline and file-reset behavior.

### Requirement: Command submission stops when it cannot continue safely
**Reason**: Terminal engine submission failure is now owned by `console-commands`.
**Migration**: Use `console-commands` for terminal failure behavior.
