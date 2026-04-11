# command-submission Specification

## Purpose
File-backed Skyrim VR console command submission through the validated ExecuteCommand path.

## Requirements
### Requirement: Plugin can submit file-backed console commands on Skyrim VR
The system MUST support a local inbox file for agent-authored console commands, MUST submit unread newline-terminated commands through the validated Skyrim VR `ExecuteCommand` path, and MUST record only commands that traverse that engine submission seam as `[console-submit]` activity.

#### Scenario: Unread command is submitted
- **WHEN** a new newline-terminated command is written to the configured inbox file on Skyrim VR
- **AND** the validated VR `ExecuteCommand` path and console UI are available
- **THEN** the plugin submits that command through the game callback
- **AND** it records a `[console-submit]` activity line

### Requirement: Command submission starts only on the validated submission path
The system MUST start command submission only when the active runtime is Skyrim VR and the required `ExecuteCommand` callback seam has been validated.

#### Scenario: Submission path is not available
- **WHEN** the active runtime is not Skyrim VR or the `ExecuteCommand` callback seam cannot be validated
- **THEN** the command submission path is not started
- **AND** no commands are submitted through the engine callback

### Requirement: Command submission defers safely until the console UI is available
The system MUST keep pending commands queued when the validated submission path exists but the console UI is not ready.

#### Scenario: Console menu is closed
- **WHEN** pending inbox commands exist but the console menu is not open
- **THEN** the plugin keeps those commands queued
- **AND** it retries later instead of dropping them

### Requirement: Command submission resets stale queued work when the inbox file changes identity
The system MUST discard stale buffered commands when the inbox file is replaced, truncated, or removed.

#### Scenario: Inbox file is replaced
- **WHEN** the watched inbox resolves to a different file identity than the previously observed file
- **THEN** the plugin clears any pending commands from the previous file
- **AND** it rereads from the new file head

### Requirement: Command submission stops when it cannot continue safely
The system MUST stop engine command submission after a terminal submission failure.

#### Scenario: Submission path later fails
- **WHEN** the command submission path encounters a terminal failure after startup
- **THEN** the plugin logs the failure
- **AND** later engine command submission attempts do not proceed
