## MODIFIED Requirements

### Requirement: Prefix lookup output is bounded, ordered, and includes source

The system MUST order prefix lookup matches by Editor ID, MUST emit at most the effective result limit, MUST include the source plugin on each emitted match line, and MUST state when additional matches were truncated.

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

#### Scenario: Prefix lookup match output includes the owning source plugin

- **WHEN** the prefix lookup emits a match line
- **THEN** that line includes `source="<plugin>"`
- **AND** that value is the source plugin filename for the matched form

#### Scenario: Prefix lookup reports truncation

- **WHEN** the prefix lookup finds more matches than the effective result limit
- **THEN** the plugin emits a local response line stating that the result set was truncated
