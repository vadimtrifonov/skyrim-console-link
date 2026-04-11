## Why

`cdbg.lookup` is useful for exact Editor ID inspection, but the next practical local command is a bounded prefix search over the same native Editor ID map. Adding that companion command expands the local debugging surface without widening the engine submission path.

## What Changes

- Add `cdbg.lookup-prefix <prefix> [limit]` as a plugin-local command.
- Support bounded prefix matching over the native Editor ID map.
- Use built-in default limit `10` when `limit` is omitted.
- Return clear local output when the result set is truncated by the effective limit.
- Keep the command on the local path without engine submission.

## Capabilities

### New Capabilities
- `local-command-lookup-prefix`: Plugin-local prefix lookup command with bounded ordered output.

### Modified Capabilities
