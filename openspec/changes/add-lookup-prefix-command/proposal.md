## Why

`cdbg.lookup` is useful for exact Editor ID inspection, but the next practical local command is a bounded prefix search over the same native Editor ID map. Adding that companion command expands the local debugging surface without widening the engine submission path.

## What Changes

- Add `cdbg.lookup-prefix <prefix> [limit]` as a plugin-local command.
- Support bounded prefix matching over the native Editor ID map.
- Return clear local output when the result set is truncated by policy or limit.
- Keep the command on the local path without engine submission.

## Capabilities

### New Capabilities

### Modified Capabilities
- `local-command-lookup`: Local lookup support expands from exact match to include bounded prefix search.

## Impact

- Affects local command parsing, Editor ID lookup behavior, and local response formatting.
- Requires additional automated coverage for prefix matching, limits, and truncated result output.
