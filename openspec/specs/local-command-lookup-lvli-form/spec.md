# local-command-lookup-lvli-form Specification

## Purpose

Plugin-local `cdbg.lookup-lvli-form` command support for direct leveled-list inspection by plugin-qualified local form ID.

## Requirements

### Requirement: Plugin can execute cdbg.lookup-lvli-form locally

The system MUST support `cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]` as a plugin-local command that resolves one exact plugin-qualified local form ID and inspects the resolved leveled list's immediate entries.

#### Scenario: Lookup-lvli-form command is handled locally

- **WHEN** the plugin handles `cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]` on the local command path
- **THEN** the plugin resolves the exact plugin-qualified local form ID locally
- **AND** it emits local response lines describing the leveled list and its direct entries

### Requirement: Lookup-lvli-form validates its argument shape

The system MUST reject malformed `cdbg.lookup-lvli-form` usage with a readable local response line.

#### Scenario: Lookup-lvli-form arguments are invalid

- **WHEN** the user submits `cdbg.lookup-lvli-form` without a form-key argument
- **THEN** the plugin emits `usage: cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]`

#### Scenario: Lookup-lvli-form key is malformed

- **WHEN** the user submits `cdbg.lookup-lvli-form nope`
- **THEN** the plugin emits `usage: cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]`

#### Scenario: Lookup-lvli-form limit is zero

- **WHEN** the user submits `cdbg.lookup-lvli-form Skyrim.esm|0003DF19 0`
- **THEN** the plugin emits `usage: cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]`

#### Scenario: Lookup-lvli-form limit is not numeric

- **WHEN** the user submits `cdbg.lookup-lvli-form Skyrim.esm|0003DF19 nope`
- **THEN** the plugin emits `usage: cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]`

### Requirement: Lookup-lvli-form distinguishes miss from wrong type

The system MUST report a missing exact form separately from a resolved non-`LVLI` form.

#### Scenario: Lookup-lvli-form misses

- **WHEN** the user submits `cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]`
- **AND** no form resolves for that exact plugin-qualified local form ID
- **THEN** the plugin emits `lookup-lvli-form "<plugin>|<local-formid>" -> miss`

#### Scenario: Lookup-lvli-form resolves a non-LVLI form

- **WHEN** the user submits `cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]`
- **AND** the exact form resolves a form that is not `LVLI`
- **THEN** the plugin emits a wrong-type response line for that form
- **AND** that line includes `form=0x...`
- **AND** that line includes `type=<TYPE>`
- **AND** that line includes `editorID="..."`
- **AND** that line includes `source="<plugin>"`

### Requirement: Lookup-lvli-form uses plugin-qualified local form IDs

The system MUST treat the command argument as a plugin-qualified local form ID, not as a runtime-loaded form ID or selected-reference ID.

#### Scenario: Lookup-lvli-form uses a plugin-qualified local form ID

- **WHEN** the user submits `cdbg.lookup-lvli-form Skyrim.esm|0003DF19`
- **THEN** the plugin resolves local form `0003DF19` in `Skyrim.esm`
- **AND** it does not interpret that token as a runtime-loaded form ID

### Requirement: Lookup-lvli-form output is bounded and preserves list order

The system MUST preserve immediate-entry order, MUST emit at most the effective result limit, MUST treat a nested `LVLI` reference as one emitted entry rather than expanding it recursively, and MUST state when additional immediate entries were truncated.

#### Scenario: Lookup-lvli-form uses the built-in default limit

- **WHEN** the user submits `cdbg.lookup-lvli-form <plugin>|<local-formid>` without an explicit limit
- **THEN** the plugin caps emitted direct-entry lines at `10`

#### Scenario: Lookup-lvli-form uses the requested limit

- **WHEN** the user submits `cdbg.lookup-lvli-form <plugin>|<local-formid> <limit>`
- **THEN** the plugin caps emitted direct-entry lines at `<limit>`

#### Scenario: Lookup-lvli-form preserves direct-entry order before truncation

- **WHEN** the plugin emits multiple entry lines for a resolved leveled list
- **THEN** those entry lines are emitted in the list's immediate-entry order
- **AND** the plugin emits no more than the effective result limit

#### Scenario: Lookup-lvli-form reports nested LVLI as one entry

- **WHEN** an immediate entry points to another `LVLI`
- **THEN** the plugin emits one entry line for that referenced `LVLI`
- **AND** it does not emit that referenced list's child entries

#### Scenario: Lookup-lvli-form reports truncation

- **WHEN** the leveled list contains more immediate entries than the effective result limit
- **THEN** the plugin emits a local response line stating that the result set was truncated

### Requirement: Lookup-lvli-form output includes list and entry metadata

The system MUST emit enough metadata to identify the resolved leveled list and each emitted direct entry.

#### Scenario: Lookup-lvli-form summary identifies the resolved list

- **WHEN** the exact plugin-qualified local form ID resolves a leveled list
- **THEN** the summary response line includes `form=0x...`
- **AND** that line includes `editorID="..."`
- **AND** that line includes `entries=<count>`
- **AND** that line includes `source="<plugin>"`

#### Scenario: Lookup-lvli-form entry output includes identifying metadata

- **WHEN** the plugin emits a direct-entry line
- **THEN** that line includes `level=<n>`
- **AND** that line includes `count=<n>`
- **AND** that line includes `form=0x...`
- **AND** that line includes `type=<TYPE>`
- **AND** that line includes `source="<plugin>"`

#### Scenario: Lookup-lvli-form entry output includes editor ID when available

- **WHEN** the emitted direct-entry form has an Editor ID
- **THEN** that direct-entry line includes `editorID="<EditorID>"`

#### Scenario: Lookup-lvli-form entry output includes full name when available

- **WHEN** the emitted direct-entry form has a non-empty full name
- **THEN** that direct-entry line includes `name="<name>"`
