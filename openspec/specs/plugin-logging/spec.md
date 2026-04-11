# plugin-logging Specification

## Purpose

Plugin lifecycle and activity logging.

## Requirements

### Requirement: Plugin load requires lifecycle and activity logs

The system MUST initialize `ConsoleLink.log` for lifecycle state and `ConsoleLink.console.log` for mirrored console activity during plugin load, and MUST fail plugin startup if either log cannot be initialized.

#### Scenario: Plugin loads successfully
- **WHEN** the plugin is loaded through SKSE
- **THEN** it initializes `ConsoleLink.log`
- **AND** it initializes `ConsoleLink.console.log`

#### Scenario: Logging initialization fails
- **WHEN** lifecycle logging or activity logging cannot initialize
- **THEN** plugin startup fails

### Requirement: Console activity records are written to the activity log
The system MUST write feature-owned console activity records to `ConsoleLink.console.log`.

#### Scenario: Feature emits a console activity record
- **WHEN** a feature records a console activity event
- **THEN** that record is written to `ConsoleLink.console.log`

