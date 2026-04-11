# plugin-settings Specification

## Purpose
INI-backed startup configuration for runtime feature flags, paths, and diagnostics.

## Requirements
### Requirement: Plugin reads default configuration from the SKSE plugin directory
The system MUST load configuration from `Data/SKSE/Plugins/ConsoleLink.ini` and use that configuration to control runtime feature flags and paths.

#### Scenario: Default settings path is resolved
- **WHEN** startup requests the default settings file
- **THEN** the resolved path points to `Data/SKSE/Plugins/ConsoleLink.ini`

### Requirement: Settings parsing is forgiving and defaults safely
The system MUST parse console logging, display, and command submission settings from the INI file, MUST provide built-in default inbox and outbox filenames under the `ConsoleLink` name, and MUST disable display or command submission clearly when a configured path is empty or invalid.

#### Scenario: Missing outbox path uses the built-in default
- **WHEN** `OutboxPath` is omitted from the INI file
- **THEN** display remains enabled
- **AND** the built-in default outbox filename is `ConsoleLink.outbox.txt`

#### Scenario: Missing inbox path uses the built-in default
- **WHEN** `InboxPath` is omitted from the INI file
- **THEN** command submission remains enabled
- **AND** the built-in default inbox filename is `ConsoleLink.inbox.txt`

### Requirement: Enabled relative inbox and outbox paths resolve under the SKSE log directory
The system MUST resolve enabled relative file-backed display and command-submission paths under the active SKSE log directory during bootstrap, and MUST fail plugin startup if that directory cannot be resolved when such a path is required.

#### Scenario: Relative handoff paths are configured
- **WHEN** display or command submission is enabled
- **AND** the configured inbox or outbox path is relative
- **THEN** bootstrap resolves that path under the SKSE log directory

#### Scenario: Required relative handoff path resolution fails
- **WHEN** display or command submission is enabled
- **AND** the configured inbox or outbox path is relative
- **AND** the standard SKSE log directory cannot be resolved
- **THEN** plugin startup fails

### Requirement: Startup logging reports the active runtime configuration
The system MUST write a startup log summary of the configuration it will actually use at runtime so an operator can confirm which file-backed features are enabled and which paths and limits are in effect.

#### Scenario: Startup reports runtime configuration
- **WHEN** bootstrap completes configuration loading
- **THEN** startup logging includes whether input logging, output logging, display, and command submission are enabled or disabled
- **AND** startup logging includes the inbox path and outbox path the plugin will actually use at runtime
- **AND** startup logging includes the configured per-tick display-line and command limits
