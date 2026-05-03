## MODIFIED Requirements

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
- **AND** that line includes `editorID="..."`
- **AND** that line includes `source="<plugin>"`

### Requirement: Lookup-form hit output includes identifying metadata when available

The system MUST emit enough metadata to identify the resolved form and SHOULD include optional identifiers when they exist.

#### Scenario: Lookup-form hit output includes Editor ID when available

- **WHEN** the exact form resolves and has an Editor ID
- **THEN** the hit response line includes `editorID="<EditorID>"`

#### Scenario: Lookup-form hit output includes full name when available

- **WHEN** the exact form resolves and has a non-empty full name
- **THEN** the hit response line includes `name="<name>"`
