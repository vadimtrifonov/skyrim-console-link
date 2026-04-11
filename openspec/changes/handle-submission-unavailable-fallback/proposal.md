## Why

Today the inbox path is narrower than it should be when engine command submission is unavailable. Local commands should remain useful, and non-local lines should fail in a visible, non-blocking way instead of stalling the inbox behind an unavailable backend.

## What Changes

- Keep the inbox service active even when the validated engine submission seam is unavailable.
- Continue to handle supported local `cdbg.*` commands on the plugin-local path.
- Drain non-local inbox lines instead of blocking later local commands behind an unavailable submission backend.
- Record a canonical local outcome for drained-but-not-submitted non-local lines.
- Emit a clear lifecycle warning that explains why engine submission is unavailable.

## Capabilities

### New Capabilities

### Modified Capabilities
- `command-submission`: Inbox behavior changes when the validated submission seam is unavailable.
- `local-commands`: Local command availability remains intact even when engine submission cannot run.

## Impact

- Affects inbox queue policy, canonical activity logging, and failure handling when runtime validation fails.
- Requires updates to runtime logging, queue-drain behavior, and related automated tests.
