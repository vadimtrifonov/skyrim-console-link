## MODIFIED Requirements

### Requirement: Locally handled command activity is recorded as local activity

The system MUST record locally handled `cdbg.` request and response lines through the `[local]` activity channel rather than through `console-*` activity channels.

#### Scenario: Local command request and response are handled
- **WHEN** the plugin handles a `cdbg.` request or response line locally
- **THEN** that line is recorded as `[local]`
- **AND** it is not recorded as `[console-submit]`
