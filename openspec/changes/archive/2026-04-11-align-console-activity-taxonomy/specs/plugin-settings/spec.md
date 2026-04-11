## MODIFIED Requirements

### Requirement: Settings parsing is forgiving and defaults safely
The system MUST parse console logging, print, and command submission settings from the INI file, MUST provide built-in default inbox and outbox filenames under the `ConsoleLink` name, and MUST disable print or command submission clearly when a configured path is empty or invalid.

#### Scenario: Missing print outbox path uses the built-in default
- **WHEN** `OutboxPath` is omitted from the canonical print section in the INI file
- **THEN** print remains enabled
- **AND** the built-in default outbox filename is `ConsoleLink.outbox.txt`

### Requirement: Canonical print config names are accepted
The system MUST accept `EnableConsolePrint` and `[Print]` as the canonical config names for the plugin-authored print path.

#### Scenario: Canonical print config names are used
- **WHEN** the INI file sets `EnableConsolePrint`
- **AND** configures print settings under `[Print]`
- **THEN** the plugin loads those values for the print path
