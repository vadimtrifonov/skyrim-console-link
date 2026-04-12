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
The system MUST parse console logging and inbox settings from the INI file, MUST provide the built-in default inbox filename under the `ConsoleLink` name, and MUST disable inbox processing clearly when a configured path is empty or invalid.

#### Scenario: Missing inbox path uses the built-in default
- **WHEN** `Path` is omitted from the canonical inbox section in the INI file
- **THEN** inbox processing remains enabled
- **AND** the built-in default inbox filename is `ConsoleLink.inbox.txt`

### Requirement: Canonical inbox config names are accepted
The system MUST accept `EnableInbox` and `[Inbox]` as the canonical config names for inbox transport settings.

#### Scenario: Canonical inbox config names are used
- **WHEN** the INI file sets `EnableInbox`
- **AND** configures inbox settings under `[Inbox]`
- **THEN** the plugin loads those values for inbox transport

### Requirement: Enabled relative inbox paths resolve under the SKSE log directory
The system MUST resolve enabled relative file-backed inbox paths under the active SKSE log directory during bootstrap, and MUST fail plugin startup if that directory cannot be resolved when such a path is required.

#### Scenario: Relative inbox path is configured
- **WHEN** inbox processing is enabled
- **AND** the configured inbox path is relative
- **THEN** bootstrap resolves that path under the SKSE log directory

#### Scenario: Required relative inbox path resolution fails
- **WHEN** inbox processing is enabled
- **AND** the configured inbox path is relative
- **AND** the standard SKSE log directory cannot be resolved
- **THEN** plugin startup fails

### Requirement: Startup logging reports the active runtime configuration
The system MUST write a startup log summary of the configuration it will actually use at runtime so an operator can confirm which file-backed features are enabled and which paths and limits are in effect.

#### Scenario: Startup reports runtime configuration
- **WHEN** bootstrap completes configuration loading
- **THEN** startup logging includes whether input logging, output logging, and inbox processing are enabled or disabled
- **AND** startup logging includes the inbox path the plugin will actually use at runtime
- **AND** startup logging includes the configured per-tick inbox-line limit
