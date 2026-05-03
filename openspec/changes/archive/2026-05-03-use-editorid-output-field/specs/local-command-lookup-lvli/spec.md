## MODIFIED Requirements

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
- **THEN** that direct-entry line includes `editorID="<EditorID>"`

#### Scenario: Lookup-lvli entry output includes full name when available

- **WHEN** the emitted direct-entry form has a non-empty full name
- **THEN** that direct-entry line includes `name="<name>"`
