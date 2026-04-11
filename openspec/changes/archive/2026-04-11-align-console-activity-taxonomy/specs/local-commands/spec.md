## MODIFIED Requirements

### Requirement: Locally handled command activity is recorded as local activity
The system MUST record locally handled `cdbg.` request and response lines through the `[local]` activity channel rather than through `console-*` activity channels.

#### Scenario: Local command request and response are handled
- **WHEN** the plugin handles a `cdbg.` request or response line locally
- **THEN** that line is recorded as `[local]`
- **AND** it is not recorded as `[console-submit]`

### Requirement: Local response lines can use direct print when available
The system MUST allow supported local commands to print response lines immediately when console print is enabled and the direct print path is available.

#### Scenario: Local response line is printed immediately
- **WHEN** a supported local command produces response lines
- **AND** console print is enabled
- **AND** the direct print path is available
- **THEN** the plugin prints those response lines immediately
- **AND** it does not need to wait for outbox polling to surface them
