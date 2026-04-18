## Why

`cdbg.lookup` proves that a form resolves by Editor ID, but it does not answer what a leveled list contains at runtime. We need that missing step for Sentinel targets like `TH_IronCuirass`.

## What Changes

- Add `cdbg.lookup-lvli <EditorID> [limit]`.
- Resolve one exact Editor ID and require the resolved form to be `LVLI`.
- Emit a bounded summary plus direct-entry lines for the leveled list.
- Distinguish `miss` from wrong-type.

## Capabilities

### New Capabilities
- `local-command-lookup-lvli`: inspect a leveled list by exact Editor ID.
