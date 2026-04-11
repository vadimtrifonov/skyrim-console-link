## ADDED Requirements

### Requirement: Plugin can execute cdbg.lookup-prefix locally
The system MUST support `cdbg.lookup-prefix <prefix> [limit]` as a plugin-local command that performs a bounded Editor ID prefix lookup against the native Editor ID map.

#### Scenario: Prefix lookup command is handled locally
- **WHEN** the plugin handles `cdbg.lookup-prefix <prefix> [limit]` on the local command path
- **THEN** the plugin performs an Editor ID prefix lookup against the native Editor ID map
- **AND** it emits local response lines describing the matching results

### Requirement: Prefix lookup validates its argument shape
The system MUST reject malformed `cdbg.lookup-prefix` usage with a readable local response line.

#### Scenario: Prefix argument is missing
- **WHEN** the user submits `cdbg.lookup-prefix` without a prefix argument
- **THEN** the plugin emits `usage: cdbg.lookup-prefix <prefix> [limit]`

#### Scenario: Limit is zero
- **WHEN** the user submits `cdbg.lookup-prefix <prefix> 0`
- **THEN** the plugin emits `usage: cdbg.lookup-prefix <prefix> [limit]`

#### Scenario: Limit is not numeric
- **WHEN** the user submits `cdbg.lookup-prefix <prefix> nope`
- **THEN** the plugin emits `usage: cdbg.lookup-prefix <prefix> [limit]`

### Requirement: Prefix lookup output is bounded, ordered, and explicit about truncation
The system MUST order prefix lookup matches by Editor ID, MUST emit at most the effective result limit, and MUST state when additional matches were truncated.

#### Scenario: Prefix lookup uses the built-in default limit
- **WHEN** the user submits `cdbg.lookup-prefix <prefix>` without an explicit limit
- **THEN** the plugin caps emitted matches at `10`

#### Scenario: Prefix lookup uses the requested limit
- **WHEN** the user submits `cdbg.lookup-prefix <prefix> <limit>`
- **THEN** the plugin caps emitted matches at `<limit>`

#### Scenario: Prefix lookup emits ordered bounded results
- **WHEN** the plugin emits multiple prefix lookup matches
- **THEN** those match lines are ordered by Editor ID
- **AND** the plugin emits no more than the effective result limit

#### Scenario: Prefix lookup reports truncation
- **WHEN** the prefix lookup finds more matches than the effective result limit
- **THEN** the plugin emits a local response line stating that the result set was truncated
