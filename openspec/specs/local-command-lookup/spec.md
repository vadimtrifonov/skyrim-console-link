# local-command-lookup Specification

## Purpose

Plugin-local `cdbg.lookup` command support for exact Editor ID inspection.

## Requirements

### Requirement: Plugin can execute cdbg.lookup locally

The system MUST support `cdbg.lookup <EditorID>` as a plugin-local command that performs an exact Editor ID lookup.

#### Scenario: Lookup command is handled locally

- **WHEN** the plugin handles `cdbg.lookup <EditorID>` on the local command path
- **THEN** the plugin performs an exact Editor ID lookup
- **AND** it emits local response lines describing the result

#### Scenario: Lookup hit output includes the source plugin

- **WHEN** the exact lookup resolves a form
- **THEN** the hit response line includes `source="<plugin>"`
- **AND** that value is the source plugin filename for the resolved form

### Requirement: Lookup command validates its argument shape

The system MUST reject malformed `cdbg.lookup` usage with a readable local response line.

#### Scenario: Lookup arguments are invalid

- **WHEN** the user submits `cdbg.lookup` without exactly one editor ID argument
- **THEN** the plugin emits `usage: cdbg.lookup <EditorID>`
