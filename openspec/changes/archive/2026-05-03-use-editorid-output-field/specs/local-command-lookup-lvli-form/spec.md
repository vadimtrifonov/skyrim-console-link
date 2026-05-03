## MODIFIED Requirements

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
