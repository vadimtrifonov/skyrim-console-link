# console-commands Specification

## Purpose
Readiness-gated engine submission of admitted non-local commands on supported Skyrim SE, AE, and VR runtimes.

## Requirements

### Requirement: Console commands submit accepted lines on Skyrim SE/AE/VR
The system MUST submit admitted non-local commands to the in-game console on Skyrim SE/AE/VR when console-command readiness is active, and MUST record accepted submissions as `[console-submit]` activity.

#### Scenario: Ready console command is submitted
- **WHEN** a non-local command reaches console-command handling
- **AND** console-command readiness is active
- **THEN** the plugin submits that command to the in-game console
- **AND** it records a `[console-submit]` activity line

### Requirement: Console-command readiness requires an open console on Skyrim SE/AE/VR
The system MUST expose console-command readiness on Skyrim SE/AE/VR only when the console UI is open.

#### Scenario: Console menu is closed
- **WHEN** the console UI is not open
- **THEN** console-command readiness is not active
- **AND** no commands are submitted to the in-game console

### Requirement: Console commands reject execution while not ready
The system MUST reject routed non-local lines through `[local]` activity when console-command readiness is not active, and MUST not queue those lines for later execution.

#### Scenario: Console command arrives while the console is closed
- **WHEN** a routed non-local line reaches console-command handling
- **AND** the console UI is not open
- **THEN** the plugin records a `[local]` rejection explaining console commands are not ready
- **AND** it does not queue the line for later execution

### Requirement: Console-command readiness transitions are announced through local activity
The system MUST announce console-command readiness transitions through `[local]` activity.

#### Scenario: Console commands become ready
- **WHEN** console-command readiness becomes active
- **THEN** it records a `[local]` line stating console commands are ready

#### Scenario: Console commands become not ready
- **WHEN** console-command readiness transitions from active to not-ready
- **THEN** it records a `[local]` line stating console commands are not ready
