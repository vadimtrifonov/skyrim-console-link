# local-command-lookup-lvli Specification

## Purpose

Plugin-local `cdbg.lookup-lvli` command support for direct leveled-list inspection by exact Editor ID.

## Requirements

### Requirement: Plugin can execute cdbg.lookup-lvli locally

The system MUST support `cdbg.lookup-lvli <EditorID> [limit]` as a plugin-local command that resolves an exact Editor ID and inspects the resolved leveled list's immediate entries.

#### Scenario: Lookup-lvli command is handled locally

- **WHEN** the plugin handles `cdbg.lookup-lvli <EditorID> [limit]` on the local command path
- **THEN** the plugin resolves the exact Editor ID locally
- **AND** it emits local response lines describing the leveled list and its direct entries

### Requirement: Lookup-lvli command validates its argument shape

The system MUST reject malformed `cdbg.lookup-lvli` usage with a readable local response line.

#### Scenario: Lookup-lvli arguments are invalid

- **WHEN** the user submits `cdbg.lookup-lvli` without an editor ID argument
- **THEN** the plugin emits `usage: cdbg.lookup-lvli <EditorID> [limit]`

#### Scenario: Lookup-lvli limit is zero

- **WHEN** the user submits `cdbg.lookup-lvli <EditorID> 0`
- **THEN** the plugin emits `usage: cdbg.lookup-lvli <EditorID> [limit]`

#### Scenario: Lookup-lvli limit is not numeric

- **WHEN** the user submits `cdbg.lookup-lvli <EditorID> nope`
- **THEN** the plugin emits `usage: cdbg.lookup-lvli <EditorID> [limit]`

### Requirement: Lookup-lvli distinguishes miss from wrong type

The system MUST report a missing Editor ID separately from a resolved non-`LVLI` form.

#### Scenario: Lookup-lvli misses

- **WHEN** the user submits `cdbg.lookup-lvli <EditorID> [limit]`
- **AND** no form resolves for that exact Editor ID
- **THEN** the plugin emits `lookup-lvli "<EditorID>" -> miss`

#### Scenario: Lookup-lvli resolves a non-LVLI form

- **WHEN** the user submits `cdbg.lookup-lvli <EditorID> [limit]`
- **AND** the exact Editor ID resolves a form that is not `LVLI`
- **THEN** the plugin emits a wrong-type response line for that form
- **AND** that line includes `form=0x...`
- **AND** that line includes `type=<TYPE>`
- **AND** that line includes `source="<plugin>"`

### Requirement: Lookup-lvli output is bounded and preserves list order

The system MUST preserve immediate-entry order, MUST emit at most the effective result limit, MUST treat a nested `LVLI` reference as one emitted entry rather than expanding it recursively, and MUST state when additional immediate entries were truncated.

#### Scenario: Lookup-lvli uses the built-in default limit

- **WHEN** the user submits `cdbg.lookup-lvli <EditorID>` without an explicit limit
- **THEN** the plugin caps emitted direct-entry lines at `10`

#### Scenario: Lookup-lvli uses the requested limit

- **WHEN** the user submits `cdbg.lookup-lvli <EditorID> <limit>`
- **THEN** the plugin caps emitted direct-entry lines at `<limit>`

#### Scenario: Lookup-lvli preserves direct-entry order before truncation

- **WHEN** the plugin emits multiple entry lines for a resolved leveled list
- **THEN** those entry lines are emitted in the list's immediate-entry order
- **AND** the plugin emits no more than the effective result limit

#### Scenario: Lookup-lvli reports nested LVLI as one entry

- **WHEN** an immediate entry points to another `LVLI`
- **THEN** the plugin emits one entry line for that referenced `LVLI`
- **AND** it does not emit that referenced list's child entries

#### Scenario: Lookup-lvli reports truncation

- **WHEN** the leveled list contains more immediate entries than the effective result limit
- **THEN** the plugin emits a local response line stating that the result set was truncated

### Requirement: Lookup-lvli output includes list and entry metadata

The system MUST emit enough metadata to identify the resolved leveled list and each emitted direct entry.

#### Scenario: Lookup-lvli summary identifies the resolved list

- **WHEN** the exact Editor ID resolves a leveled list
- **THEN** the summary response line includes `form=0x...`
- **AND** that line includes `entries=<count>`
- **AND** that line includes `source="<plugin>"`

#### Scenario: Lookup-lvli entry output includes identifying metadata

- **WHEN** the plugin emits a direct-entry line
- **THEN** that line includes `level=<n>`
- **AND** that line includes `count=<n>`
- **AND** that line includes `form=0x...`
- **AND** that line includes `type=<TYPE>`
- **AND** that line includes `source="<plugin>"`

#### Scenario: Lookup-lvli entry output includes editor ID when available

- **WHEN** the emitted direct-entry form has an Editor ID
- **THEN** that direct-entry line includes `editor="<EditorID>"`

#### Scenario: Lookup-lvli entry output includes full name when available

- **WHEN** the emitted direct-entry form has a non-empty full name
- **THEN** that direct-entry line includes `name="<name>"`
