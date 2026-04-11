## MODIFIED Requirements

### Requirement: Plugin can submit file-backed console commands on Skyrim VR
The system MUST support a local inbox file for agent-authored console commands, MUST submit unread newline-terminated commands through the validated Skyrim VR `ExecuteCommand` path, and MUST record only commands that traverse that engine submission seam as `[console-submit]` activity.

#### Scenario: Unread command is submitted
- **WHEN** a new newline-terminated command is written to the configured inbox file on Skyrim VR
- **AND** the validated VR `ExecuteCommand` path and console UI are available
- **THEN** the plugin submits that command through the game callback
- **AND** it records a `[console-submit]` activity line
