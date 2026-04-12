## ADDED Requirements

### Requirement: Console commands submit through the validated engine seam
The system MUST submit admitted non-local commands through the validated Skyrim VR `ExecuteCommand` path when console-command readiness is active, and MUST record only commands that traverse that seam as `[console-submit]` activity.

#### Scenario: Ready console command is submitted
- **WHEN** a non-local command reaches console-command handling
- **AND** console-command readiness is active
- **THEN** the plugin submits that command through the game callback
- **AND** it records a `[console-submit]` activity line

### Requirement: Console-command readiness requires the validated path and open console
The system MUST expose console-command readiness only when the active runtime is Skyrim VR, the required `ExecuteCommand` callback seam has been validated, and the console UI is open.

#### Scenario: Submission path is not available
- **WHEN** the active runtime is not Skyrim VR or the `ExecuteCommand` callback seam cannot be validated
- **THEN** console-command readiness is not active
- **AND** no commands are submitted through the engine callback

#### Scenario: Console menu is closed
- **WHEN** the validated `ExecuteCommand` path exists but the console UI is not open
- **THEN** console-command readiness is not active
- **AND** no commands are submitted through the engine callback

### Requirement: Console commands reject execution while not ready
The system MUST reject routed non-local lines through `[local]` activity when console-command readiness is not active, and MUST not queue those lines for later execution.

#### Scenario: Console command arrives while the console is closed
- **WHEN** a routed non-local line reaches console-command handling
- **AND** the console UI is not open
- **THEN** the plugin records a `[local]` rejection explaining console commands are not ready
- **AND** it does not queue the line for later execution

#### Scenario: Console command arrives while the validated path is unavailable
- **WHEN** a routed non-local line reaches console-command handling
- **AND** the validated `ExecuteCommand` path is unavailable
- **THEN** the plugin records a `[local]` rejection explaining console command submission is unavailable
- **AND** it does not queue the line for later execution

### Requirement: Console-command readiness transitions are announced through local activity
The system MUST announce console-command readiness transitions through `[local]` activity.

#### Scenario: Console commands become ready
- **WHEN** console-command readiness becomes active
- **THEN** it records a `[local]` line stating console commands are ready

#### Scenario: Console commands become unavailable
- **WHEN** console-command readiness transitions from active to inactive
- **THEN** it records a `[local]` line stating console commands are unavailable

### Requirement: Console-command processing stops after terminal submission failure
The system MUST stop later engine command submission attempts after a terminal submission failure.

#### Scenario: Submission path later fails
- **WHEN** console-command handling encounters a terminal submission failure after startup
- **THEN** the plugin logs the failure
- **AND** later engine command submission attempts do not proceed
