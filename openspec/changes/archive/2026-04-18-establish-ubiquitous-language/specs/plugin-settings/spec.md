## MODIFIED Requirements

### Requirement: Plugin reads default settings from the SKSE plugin directory
The system MUST load settings from `Data/SKSE/Plugins/ConsoleLink.ini` and use those settings to control runtime feature flags and paths.

#### Scenario: Default settings path is resolved
- **WHEN** startup requests the default settings file
- **THEN** the resolved path points to `Data/SKSE/Plugins/ConsoleLink.ini`

### Requirement: Startup logging reports the active runtime settings
The system MUST write a startup log summary of the settings it will actually use at runtime so an operator can confirm which file-backed features are enabled and which paths and limits are in effect.

#### Scenario: Startup reports runtime settings
- **WHEN** bootstrap completes settings loading
- **THEN** startup logging includes whether input logging, output logging, and inbox processing are enabled or disabled
- **AND** startup logging includes the inbox path the plugin will actually use at runtime
- **AND** startup logging includes the configured per-tick inbox-line limit
