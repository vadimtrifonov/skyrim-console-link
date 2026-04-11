# console-print Specification

## Purpose
File-backed Skyrim VR console print lines that do not execute as commands.

## Requirements
### Requirement: Plugin can print file-backed lines on Skyrim VR without executing them
The system MUST support a local print outbox file for plugin-authored console lines and MUST print unread newline-terminated lines into the in-game console on Skyrim VR without treating them as submitted commands.

#### Scenario: Unread print line is shown
- **WHEN** a new newline-terminated line is written to the configured print outbox file on Skyrim VR
- **THEN** the plugin prints that line into the in-game console with the `ConsoleLink:` prefix
- **AND** it records a `[console-print]` activity line

### Requirement: Print starts only when print prerequisites are available
The system MUST start the print path only when the active runtime is Skyrim VR and the print service has the startup services it requires.

#### Scenario: Print path is not available
- **WHEN** the active runtime is not Skyrim VR or required print startup services are unavailable
- **THEN** the print path is not started
- **AND** no in-game print lines are emitted through that path

### Requirement: Print state resets cleanly when the outbox file changes identity
The system MUST discard stale buffered print work when the outbox file is replaced, truncated, or removed.

#### Scenario: Outbox file is replaced
- **WHEN** the watched print outbox resolves to a different file identity than the previously observed file
- **THEN** the plugin clears any pending print lines from the previous file
- **AND** it rereads from the new file head

### Requirement: Print stops when it cannot continue safely
The system MUST stop the active print path after a plugin-side print failure.

#### Scenario: Print path later fails
- **WHEN** the active print path encounters a plugin-side failure
- **THEN** the plugin logs the failure
- **AND** further print output does not proceed through that path
