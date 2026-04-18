## Why

`cdbg.lookup` and `cdbg.lookup-lvli` work well when a target has a stable Editor ID, but the remaining runtime-debug surfaces now include important vanilla and mixed-source leveled lists that are easiest to identify by plugin-qualified form key instead.

We need one exact form-inspection command and one leveled-list dump command that accept `<plugin>|<local-formid>` so we can inspect runtime state for targets like `Skyrim.esm|0003DF19` without depending on Editor ID discovery.

## What Changes

- Add `cdbg.lookup-form <plugin>|<local-formid>`.
- Add `cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]`.
- Resolve one exact plugin-qualified form key locally.
- Report exact-form hits with identifying metadata.
- Report bounded direct-entry dumps for resolved `LVLI` forms.
- Distinguish malformed key, miss, and wrong-type outcomes clearly.

## Capabilities

### New Capabilities
- `local-command-lookup-form`: inspect one exact form by plugin-qualified form key.
- `local-command-lookup-lvli-form`: inspect one exact leveled list by plugin-qualified form key.
