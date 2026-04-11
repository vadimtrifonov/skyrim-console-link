# console-observation Specification

## Purpose
Passive Skyrim VR console input and output observation for the initial debugging milestone.

## Requirements
### Requirement: Plugin captures visible console output on Skyrim VR
The system MUST passively capture visible console output on Skyrim VR and record it as `[console-out]`, reserving that `console-*` activity channel for output observed on the validated console seam without suppressing the original engine output.

#### Scenario: Visible console line is mirrored
- **WHEN** the game prints a visible console line through the validated VR console output seam
- **THEN** the original line still appears in the in-game console
- **AND** the plugin records that line as `[console-out]`

### Requirement: Plugin captures accepted console input on Skyrim VR
The system MUST passively capture accepted console input on Skyrim VR and record it as `[console-in]` after the validated callback returns, reserving that `console-*` activity channel for input observed on the validated console seam.

#### Scenario: Accepted command is mirrored
- **WHEN** the validated VR `ExecuteCommand` callback accepts a console command
- **THEN** the original command path still runs
- **AND** the plugin records the accepted command as `[console-in]`

### Requirement: Plugin output is not mirrored as passive output
The system MUST not record plugin-authored in-game console output as passive `[console-out]` activity because plugin-owned print lines are not passive console observation.

#### Scenario: Plugin-authored in-game console output is printed
- **WHEN** the plugin prints a line through its own in-game console path
- **THEN** no duplicate `[console-out]` record is emitted for that same event

### Requirement: Observation uses only validated hook sites
The system MUST install an observational hook only when the required hook site has been validated for the active runtime.

#### Scenario: Hook site cannot be validated
- **WHEN** the runtime is not Skyrim VR or the expected hook prologue does not match
- **THEN** that observational hook is not installed
- **AND** no observation records are emitted from that hook

### Requirement: Observation failures disable only the affected observation path
The system MUST stop an observational path after a plugin-side failure without emitting further records from that path.

#### Scenario: Installed observation path later fails
- **WHEN** an installed observational path encounters a plugin-side failure
- **THEN** the plugin logs the failure
- **AND** that observational path stops emitting records
