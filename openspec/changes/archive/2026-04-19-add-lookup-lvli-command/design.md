## Context

We need one local command that bridges exact Editor ID lookup to leveled-list entry inspection.

## Decisions

1. `LocalCommandProcessor` routes `cdbg.lookup-lvli`.
It keeps readiness gating and dispatches the command on the local path.

2. A new `LookupLvliCommand` leaf module owns execution.
It parses `EditorID` and optional `limit`, resolves the form, checks `LVLI`, reads the target list's immediate entries, and formats response lines.

3. Nested leveled lists are not expanded.
If an immediate entry points to another `LVLI`, the command emits that referenced `LVLI` as one entry line and does not enumerate its child entries. Entry order stays in runtime order.

4. Result limiting stays local to `LookupLvliCommand`.
The built-in default is `10`. The command counts the full immediate-entry set first, then applies the output limit and reports truncation when entries were omitted.
