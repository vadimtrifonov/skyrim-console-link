# console-capture Specification

## Purpose
Passive Skyrim VR console input and output capture for the initial debugging milestone.

## Requirements
### Requirement: Plugin captures visible console output on Skyrim VR
The system MUST passively capture visible console output on Skyrim VR and record it as `[console-out]`, reserving that `console-*` activity channel for output captured on the validated console seam without suppressing the original engine output.

#### Scenario: Visible console line is mirrored
- **WHEN** the game prints a visible console line through the validated VR console output seam
- **THEN** the original line still appears in the in-game console
- **AND** the plugin records that line as `[console-out]`

### Requirement: Plugin captures accepted console input on Skyrim VR
The system MUST passively capture accepted console input on Skyrim VR and record it as `[console-in]` after the validated callback returns, reserving that `console-*` activity channel for input captured on the validated console seam.

#### Scenario: Accepted command is mirrored
- **WHEN** the validated VR `ExecuteCommand` callback accepts a console command
- **THEN** the original command path still runs
- **AND** the plugin records the accepted command as `[console-in]`

### Requirement: Capture uses only validated hook sites
The system MUST install a capture hook only when the required hook site has been validated for the active runtime.

#### Scenario: Hook site cannot be validated
- **WHEN** the runtime is not Skyrim VR or the expected hook prologue does not match
- **THEN** that capture hook is not installed
- **AND** no capture records are emitted from that hook

### Requirement: Capture failures disable only the affected capture path
The system MUST stop a capture path after a plugin-side failure without emitting further records from that path.

#### Scenario: Installed capture path later fails
- **WHEN** an installed capture path encounters a plugin-side failure
- **THEN** the plugin logs the failure
- **AND** that capture path stops emitting records
