## ADDED Requirements

### Requirement: Inbox is a live transport
The system MUST treat the inbox as a live transport, MUST clear existing inbox contents once at startup so that the inbox begins empty, MUST announce inbox readiness only after the inbox is known empty, and MUST accept only newline-terminated lines appended after that startup clear.

#### Scenario: Startup begins with pre-existing inbox content
- **WHEN** ConsoleLink starts
- **AND** the inbox already contains newline-terminated lines
- **THEN** those pre-existing lines are removed by one inbox clear operation
- **AND** only later appended lines are eligible for handling

#### Scenario: Inbox file is rewritten during runtime
- **WHEN** the watched inbox file is replaced, truncated, or removed and later recreated after startup inbox readiness is already established
- **THEN** ConsoleLink discards buffered state from the earlier file contents
- **AND** current contents in the rewritten or recreated inbox are treated as new live input rather than as stale startup backlog

### Requirement: Inbox routes post-baseline lines by command class
The system MUST route post-baseline `cdbg.` lines toward local-command handling and MUST route other post-baseline lines toward console-command handling.

#### Scenario: Local command line is routed
- **WHEN** a new post-baseline line starts with the `cdbg.` prefix
- **THEN** ConsoleLink routes that line toward local-command handling

#### Scenario: Non-local command line is routed
- **WHEN** a new post-baseline line does not start with the `cdbg.` prefix
- **THEN** ConsoleLink routes that line toward console-command handling

### Requirement: Inbox session lifecycle is announced through local activity
The system MUST announce inbox session resets through `[local]` activity.

#### Scenario: Inbox is ready for new input
- **WHEN** ConsoleLink completes the startup inbox clear
- **AND** the inbox is known empty
- **THEN** it records a `[local]` line stating the inbox is ready for new input

#### Scenario: Startup clear cannot establish an empty inbox
- **WHEN** ConsoleLink cannot open, create, inspect, or clear the inbox file during startup
- **THEN** it records a `[local]` failure stating inbox startup failed because it could not clear existing inbox contents
- **AND** it stops inbox processing for that session
