# local-command-lookup-form Specification

## Purpose

Plugin-local `cdbg.lookup-form` command support for exact form inspection by plugin-qualified local form ID.

## Requirements

### Requirement: Plugin can execute cdbg.lookup-form locally

The system MUST support `cdbg.lookup-form <plugin>|<local-formid>` as a plugin-local command that performs one exact form lookup.

#### Scenario: Lookup-form command is handled locally

- **WHEN** the plugin handles `cdbg.lookup-form <plugin>|<local-formid>` on the local command path
- **THEN** the plugin performs one exact form lookup by plugin-qualified local form ID
- **AND** it emits local response lines describing the result

### Requirement: Lookup-form validates its argument shape

The system MUST reject malformed `cdbg.lookup-form` usage with a readable local response line.

#### Scenario: Lookup-form argument is missing

- **WHEN** the user submits `cdbg.lookup-form` without a form-key argument
- **THEN** the plugin emits `usage: cdbg.lookup-form <plugin>|<local-formid>`

#### Scenario: Lookup-form key is malformed

- **WHEN** the user submits `cdbg.lookup-form nope`
- **THEN** the plugin emits `usage: cdbg.lookup-form <plugin>|<local-formid>`

#### Scenario: Lookup-form key has a non-hex form token

- **WHEN** the user submits `cdbg.lookup-form Skyrim.esm|nothex`
- **THEN** the plugin emits `usage: cdbg.lookup-form <plugin>|<local-formid>`

### Requirement: Lookup-form distinguishes miss from hit

The system MUST report a missing exact form separately from a resolved form.

#### Scenario: Lookup-form misses

- **WHEN** the user submits `cdbg.lookup-form <plugin>|<local-formid>`
- **AND** no form resolves for that exact plugin-qualified local form ID
- **THEN** the plugin emits `lookup-form "<plugin>|<local-formid>" -> miss`

#### Scenario: Lookup-form resolves an exact form

- **WHEN** the user submits `cdbg.lookup-form <plugin>|<local-formid>`
- **AND** the exact form resolves
- **THEN** the plugin emits a hit response line for that form
- **AND** that line includes `form=0x...`
- **AND** that line includes `type=<TYPE>`
- **AND** that line includes `source="<plugin>"`

### Requirement: Lookup-form uses plugin-qualified local form IDs

The system MUST treat the command argument as a plugin-qualified local form ID, not as a runtime-loaded form ID or selected-reference ID.

#### Scenario: Lookup-form uses a plugin-qualified local form ID

- **WHEN** the user submits `cdbg.lookup-form Skyrim.esm|0003DF19`
- **THEN** the plugin resolves local form `0003DF19` in `Skyrim.esm`
- **AND** it does not interpret that token as a runtime-loaded form ID

### Requirement: Lookup-form hit output includes identifying metadata when available

The system MUST emit enough metadata to identify the resolved form and SHOULD include optional identifiers when they exist.

#### Scenario: Lookup-form hit output includes Editor ID when available

- **WHEN** the exact form resolves and has an Editor ID
- **THEN** the hit response line includes `editor="<EditorID>"`

#### Scenario: Lookup-form hit output includes full name when available

- **WHEN** the exact form resolves and has a non-empty full name
- **THEN** the hit response line includes `name="<name>"`
