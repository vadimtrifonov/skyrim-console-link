## MODIFIED Requirements

### Requirement: Plugin observes visible console output on Skyrim VR
The system MUST passively observe visible console output on Skyrim VR and record it as `[console-out]` without suppressing the original engine output.

#### Scenario: Visible console line is mirrored
- **WHEN** the game prints a visible console line through the validated VR console output seam
- **THEN** the original line still appears in the in-game console
- **AND** the plugin records that line as `[console-out]`

### Requirement: Plugin observes accepted console input on Skyrim VR
The system MUST passively observe accepted console input on Skyrim VR and record it as `[console-in]` after the validated callback returns.

#### Scenario: Accepted command is mirrored
- **WHEN** the validated VR `ExecuteCommand` callback accepts a console command
- **THEN** the original command path still runs
- **AND** the plugin records the accepted command as `[console-in]`

### Requirement: Observation uses only validated hook sites
The system MUST install an observation hook only when the required hook site has been validated for the active runtime.

#### Scenario: Hook site cannot be validated
- **WHEN** the runtime is not Skyrim VR or the expected hook prologue does not match
- **THEN** that observational hook is not installed
- **AND** no observation records are emitted from that hook

### Requirement: Observation failures disable only the affected observation path
The system MUST stop an observation path after a plugin-side failure without emitting further records from that path.

#### Scenario: Installed observation path later fails
- **WHEN** an installed observation path encounters a plugin-side failure
- **THEN** the plugin logs the failure
- **AND** that observation path stops emitting records
