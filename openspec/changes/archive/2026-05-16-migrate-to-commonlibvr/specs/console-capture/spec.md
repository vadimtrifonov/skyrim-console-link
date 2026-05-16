## REMOVED Requirements

### Requirement: Plugin captures visible console output on Skyrim VR
**Reason**: The old wording limited the behavior to Skyrim VR instead of Skyrim SE/AE/VR.
**Migration**: Use `Plugin captures visible console output on Skyrim SE/AE/VR` for output capture behavior.

### Requirement: Plugin captures accepted console input on Skyrim VR
**Reason**: The old wording limited the behavior to Skyrim VR and named the previous accepted-input mechanism rather than the behavior to preserve.
**Migration**: Use `Plugin captures accepted console input on Skyrim SE/AE/VR` for accepted-input capture behavior.

### Requirement: Capture uses only validated hook sites
**Reason**: The old wording made a specific implementation mechanism part of the normative contract.
**Migration**: Use `Capture is fail-closed when unavailable` for unavailable-capture behavior.

## ADDED Requirements

### Requirement: Plugin captures visible console output on Skyrim SE/AE/VR
The system MUST passively capture visible console output on Skyrim SE/AE/VR, MUST record captured output as `[console-out]`, and MUST NOT suppress the original in-game console output.

#### Scenario: Visible console line is mirrored
- **WHEN** the game prints a visible console line
- **THEN** the original line still appears in the in-game console
- **AND** the plugin records that line as `[console-out]`

### Requirement: Plugin captures accepted console input on Skyrim SE/AE/VR
The system MUST passively capture manually accepted console input on Skyrim SE/AE/VR and MUST record captured input as `[console-in]` after the accepted command has been processed by the game.

#### Scenario: Accepted command is mirrored
- **WHEN** the user accepts a console command
- **THEN** the original command is processed by the game
- **AND** the plugin records the accepted command as `[console-in]` after that processing

#### Scenario: Submitted command is not duplicated as accepted input
- **WHEN** the plugin submits a command to the in-game console
- **THEN** that submitted command is not recorded as `[console-in]` solely because it was plugin-submitted

### Requirement: Capture is fail-closed when unavailable
The system MUST NOT emit console capture records from an input or output capture surface that is unavailable for the active runtime or current console state.

#### Scenario: Input capture is unavailable
- **WHEN** input capture is unavailable
- **THEN** no `[console-in]` records are emitted from input capture

#### Scenario: Output capture is unavailable
- **WHEN** output capture is unavailable
- **THEN** no `[console-out]` records are emitted from output capture
