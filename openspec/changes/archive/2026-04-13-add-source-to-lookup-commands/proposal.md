## Why

Add `source="<plugin>"` to `cdbg.lookup` and `cdbg.lookup-prefix` output.

## What Changes

- Add `source="<plugin>"` to `cdbg.lookup` hit output.
- Add `source="<plugin>"` to each `cdbg.lookup-prefix` match output.
- Leave other output lines unchanged.

## Capabilities

### New Capabilities

- None.

### Modified Capabilities

- `local-command-lookup`: add `source="<plugin>"` to hit output.
- `local-command-lookup-prefix`: add `source="<plugin>"` to match output.

## Impact

- Affected code: `src/local_commands/LookupCommand.cpp`, `src/local_commands/LookupPrefixCommand.cpp`
- Affected tests: `tests/LookupCommandTests.cpp`, `tests/LookupPrefixCommandTests.cpp`
