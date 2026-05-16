# inbox Specification

## Purpose
Session-scoped inbox transport, startup clear, runtime rewrite ingestion, and routing of accepted input lines.

## Requirements
### Requirement: Inbox startup clear establishes readiness
The system MUST perform one startup inbox clear before processing inbox input, MUST announce inbox readiness only after the inbox is known empty, and MUST stop inbox processing for the session if the clear cannot establish an empty inbox.

#### Scenario: Startup begins with pre-existing inbox content
- **WHEN** the plugin starts
- **AND** the inbox already contains newline-terminated lines
- **THEN** the plugin removes those pre-existing lines during the startup inbox clear
- **AND** only later input is eligible for handling

#### Scenario: Inbox is ready for new input
- **WHEN** the startup inbox clear completes
- **AND** the inbox is known empty
- **THEN** the plugin records a `[local]` line stating the inbox is ready for new input

#### Scenario: Startup clear cannot establish an empty inbox
- **WHEN** the plugin cannot open, create, inspect, or clear the inbox file during startup
- **THEN** the plugin records a `[local]` failure stating inbox startup failed because it could not clear existing inbox contents
- **AND** it stops inbox processing for that session

### Requirement: Inbox accepts appended lines after readiness
The system MUST accept only complete newline-terminated lines appended to the inbox after inbox readiness.

#### Scenario: Line is appended after readiness
- **WHEN** the plugin has announced inbox readiness
- **AND** a newline-terminated line is appended to the watched inbox file
- **THEN** the plugin accepts that line as live input

### Requirement: Inbox accepts stable file rewrites after readiness
After inbox readiness, the system MUST treat detected overwrites of the watched inbox file as whole-file input. It MUST wait for stable rewritten contents before accepting lines. It MUST accept non-empty lines from the stable file, including a final line without a trailing newline. It MUST discard unprocessed input from earlier file contents. It MUST NOT replay unchanged rewritten contents unless another rewrite occurs.

#### Scenario: Inbox file is rewritten during runtime
- **WHEN** the watched inbox file is replaced, truncated, removed and later recreated, or overwritten in place after inbox readiness
- **THEN** the plugin discards unprocessed input from the earlier file contents
- **AND** it waits for stable rewritten contents
- **AND** it accepts non-empty lines from the stable file, including a final line without a trailing newline

#### Scenario: Same rewritten contents are polled again
- **WHEN** the plugin has already accepted lines from stable rewritten contents
- **AND** later polls see the same unchanged contents without another rewrite
- **THEN** it does not accept those lines again

#### Scenario: Later rewrite produces the same text
- **WHEN** a later rewrite occurs
- **AND** its stable contents match an earlier rewrite
- **THEN** the plugin treats the later rewrite as new input

### Requirement: Inbox routes accepted lines by command class
The system MUST route each accepted inbox line to exactly one command processor.

#### Scenario: Local command line is routed
- **WHEN** an accepted inbox line starts with the `cdbg.` prefix
- **THEN** the plugin routes that line toward local-command handling

#### Scenario: Non-local command line is routed
- **WHEN** an accepted inbox line does not start with the `cdbg.` prefix
- **THEN** the plugin routes that line toward console-command handling
