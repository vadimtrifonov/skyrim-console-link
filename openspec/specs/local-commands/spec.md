# local-commands Specification

## Purpose
Plugin-local `cdbg.` command routing, deferral, and local activity handling without engine submission.

## Requirements
### Requirement: cdbg commands stay on the local command path
The system MUST keep inbox commands with the `cdbg.` prefix on the plugin-local command path, MUST defer them until the runtime reaches `kDataLoaded`, and MUST not forward locally handled commands through the engine `ExecuteCommand` path.

#### Scenario: Local command arrives before data load
- **WHEN** the inbox contains a `cdbg.` command
- **AND** the runtime has not reached `kDataLoaded`
- **THEN** the plugin keeps that command queued
- **AND** it retries later instead of forwarding or dropping it

#### Scenario: Supported local command is handled after data load
- **WHEN** the inbox contains a supported `cdbg.` command
- **AND** the runtime has reached `kDataLoaded`
- **THEN** the plugin handles that command locally
- **AND** the command is not forwarded through the engine `ExecuteCommand` path

#### Scenario: Unknown local command name is handled locally
- **WHEN** the inbox contains an unsupported `cdbg.` command
- **THEN** the plugin produces a local response identifying the command as unknown
- **AND** the command is not forwarded through the engine `ExecuteCommand` path

### Requirement: Locally handled command activity is recorded as local activity
The system MUST record locally handled `cdbg.` request and response lines through the local activity channel rather than through engine-submission console channels.

#### Scenario: Local command request and response are handled
- **WHEN** the plugin handles a `cdbg.` request or response line locally
- **THEN** that line is recorded as `[console-local]`
- **AND** it is not recorded as `[console-submit]`

### Requirement: Local response lines can use direct display when available
The system MUST allow supported local commands to print response lines immediately when console display is enabled and the direct display path is available.

#### Scenario: Local response line is printed immediately
- **WHEN** a supported local command produces response lines
- **AND** console display is enabled
- **AND** the direct display path is available
- **THEN** the plugin prints those response lines immediately
- **AND** it does not need to wait for outbox polling to surface them
