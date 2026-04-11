# console-display Specification

## Purpose
File-backed Skyrim VR console display lines that do not execute as commands.

## Requirements
### Requirement: Plugin can display file-backed lines on Skyrim VR without executing them
The system MUST support a local display outbox file for plugin-authored console lines and MUST print unread newline-terminated lines into the in-game console on Skyrim VR without treating them as submitted commands.

#### Scenario: Unread display line is shown
- **WHEN** a new newline-terminated line is written to the configured display outbox file on Skyrim VR
- **THEN** the plugin prints that line into the in-game console with the `ConsoleLink:` prefix
- **AND** it records a `[console-display]` activity line

### Requirement: Display starts only when display prerequisites are available
The system MUST start the display path only when the active runtime is Skyrim VR and the display service has the startup services it requires.

#### Scenario: Display path is not available
- **WHEN** the active runtime is not Skyrim VR or required display startup services are unavailable
- **THEN** the display path is not started
- **AND** no in-game display lines are emitted through that path

### Requirement: Display state resets cleanly when the outbox file changes identity
The system MUST discard stale buffered display work when the outbox file is replaced, truncated, or removed.

#### Scenario: Outbox file is replaced
- **WHEN** the watched display outbox resolves to a different file identity than the previously observed file
- **THEN** the plugin clears any pending display lines from the previous file
- **AND** it rereads from the new file head

### Requirement: Display stops when it cannot continue safely
The system MUST stop the active display path after a plugin-side display failure.

#### Scenario: Display path later fails
- **WHEN** the active display path encounters a plugin-side failure
- **THEN** the plugin logs the failure
- **AND** further display output does not proceed through that path
