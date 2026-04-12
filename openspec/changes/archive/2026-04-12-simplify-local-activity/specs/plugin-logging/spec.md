## MODIFIED Requirements

### Requirement: Plugin load requires lifecycle and activity logs

The system MUST initialize `ConsoleLink.log` for lifecycle state and `ConsoleLink.activity.log` for structured activity records during plugin load, and MUST fail plugin startup if either log cannot be initialized.

#### Scenario: Plugin loads successfully
- **WHEN** the plugin is loaded through SKSE
- **THEN** it initializes `ConsoleLink.log`
- **AND** it initializes `ConsoleLink.activity.log`

#### Scenario: Logging initialization fails
- **WHEN** lifecycle logging or activity logging cannot initialize
- **THEN** plugin startup fails

### Requirement: Console activity records are written to the activity log

The system MUST write feature-owned activity records to `ConsoleLink.activity.log`.

#### Scenario: Feature emits a console activity record
- **WHEN** a feature records a console activity event
- **THEN** that record is written to `ConsoleLink.activity.log`
