---
name: console-link-operation
description: Operate Console Link in Skyrim. Use to find logs and inbox paths, send console and local commands, and read console activity.
---

# Console Link Operation

## Logs

- Standard SKSE log dir: `%USERPROFILE%\\Documents\\My Games\\<Skyrim VR|Skyrim Special Edition>\\SKSE`
- `ConsoleLink.log`: lifecycle log; use for startup warnings, enabled features, and the resolved inbox path.
- `ConsoleLink.activity.log`: activity log; use for `[local]`, `[console-in]`, `[console-out]`, and `[console-submit]` records.

## Console activity

- In-game console input is mirrored as `[console-in]`.
- In-game console output is mirrored as `[console-out]`.

## Inbox

- Default inbox path: `%USERPROFILE%\\Documents\\My Games\\<Skyrim VR|Skyrim Special Edition>\\SKSE\\ConsoleLink.inbox.txt`
- If startup logs report a different inbox path, treat the logged resolved path as the source of truth.
- The inbox is a live transport, not a backlog.
- On startup, pre-existing inbox contents are cleared.
- The inbox supports two write styles:
  - Appends: each command as a newline-terminated line.
  - Overwrites: the whole file as a complete command batch.

## Console commands

- By default, commands are treated as console commands unless they match the local-command rules first.
- Console commands require the console menu to be open; wait for `[local] console commands are ready`.
- If the console is closed, expect `[local] console commands are not ready`.
- Successful console-command submission is recorded as `[console-submit]`.

## Local commands

- Local commands become and stay available after `[local] local commands are ready`.
- Local command submissions, responses, and readiness messages appear as `[local]`.

- Available commands:
  - `cdbg.lookup <EditorID>`: exact Editor ID lookup.
  - `cdbg.lookup-prefix <prefix> [limit]`: bounded Editor ID prefix lookup. Default limit: `10`.
  - `cdbg.lookup-form <plugin>|<local-formid>`: exact form lookup by plugin-qualified local form ID such as `Skyrim.esm|0003DF19`.
  - `cdbg.lookup-lvli <EditorID> [limit]`: bounded direct leveled-list inspection by exact Editor ID. Default limit: `10`.
  - `cdbg.lookup-lvli-form <plugin>|<local-formid> [limit]`: bounded direct leveled-list inspection by plugin-qualified local form ID. Default limit: `10`.

- `cdbg.lookup-lvli` and `cdbg.lookup-lvli-form` distinguish `miss` from wrong-type and emit a summary plus direct-entry lines.
