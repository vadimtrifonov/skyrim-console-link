## MODIFIED Requirements

### Requirement: cdbg commands stay on the local command path
The system MUST keep `cdbg.` commands on the plugin-local command path once inbox routing admits them, and MUST not forward locally handled commands through the engine `ExecuteCommand` path.

#### Scenario: Supported local command is handled after data load
- **WHEN** a new supported `cdbg.` command reaches local-command handling after the runtime reaches `kDataLoaded`
- **THEN** the plugin handles that command locally
- **AND** the command is not forwarded through the engine `ExecuteCommand` path

#### Scenario: Unknown local command name is handled locally
- **WHEN** a new unsupported `cdbg.` command reaches local-command handling after the runtime reaches `kDataLoaded`
- **THEN** the plugin produces a local response identifying the command as unknown
- **AND** the command is not forwarded through the engine `ExecuteCommand` path

## ADDED Requirements

### Requirement: Local commands reject pre-ready execution
The system MUST reject routed `cdbg.` lines through `[local]` activity when local-command readiness is not active, and MUST not queue those lines for later execution.

#### Scenario: Local command arrives before local-command readiness
- **WHEN** a routed `cdbg.` line reaches local-command handling
- **AND** the runtime has not reached `kDataLoaded`
- **THEN** the plugin records a `[local]` rejection stating local commands are not ready
- **AND** it does not queue the line for later execution

### Requirement: Local command readiness is announced as local activity
The system MUST announce local-command readiness through `[local]` activity when the runtime reaches `kDataLoaded`.

#### Scenario: Local commands become ready
- **WHEN** the runtime reaches `kDataLoaded`
- **THEN** the plugin records a `[local]` line stating local commands are ready
