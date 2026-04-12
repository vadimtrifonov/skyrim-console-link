## MODIFIED Requirements

### Requirement: Plugin can execute cdbg.lookup locally

The system MUST support `cdbg.lookup <EditorID>` as a plugin-local command that performs an exact Editor ID lookup.

#### Scenario: Lookup command is handled locally

- **WHEN** the plugin handles `cdbg.lookup <EditorID>` on the local command path
- **THEN** the plugin performs an exact Editor ID lookup
- **AND** it emits local response lines describing the result

#### Scenario: Lookup hit output includes the owning source plugin

- **WHEN** the exact lookup resolves a form
- **THEN** the hit response line includes `source="<plugin>"`
- **AND** that value is the source plugin filename for the resolved form
