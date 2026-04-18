---
name: console-link
description: Operate ConsoleLink in Skyrim VR. Use to find logs and inbox paths, send local and console comamnds, and read console activity.
---

# Console Link

## Logs

- Standard SKSE log dir (VR): `%USERPROFILE%\\Documents\\My Games\\Skyrim VR\\SKSE`
- `ConsoleLink.log`: lifecycle log; use for startup warnings, enabled features, and the resolved inbox path.
- `ConsoleLink.activity.log`: activity log; use for `[local]`, `[console-in]`, `[console-out]`, and `[console-submit]` records.

## Console activity

- In-game console input is mirrored as `[console-in]`.
- In-game console output is mirrored as `[console-out]`.

## Inbox

- Default inbox path: `%USERPROFILE%\\Documents\\My Games\\Skyrim VR\\SKSE\\ConsoleLink.inbox.txt`
- If startup logs report a different inbox path, treat the logged resolved path as the source of truth.
- The inbox is a live transport, not a backlog.
- On startup, ConsoleLink clears pre-existing inbox contents once.
- Only later appended, newline-terminated lines are handled.

## Console commands

- By default, commands are treated as console commands unless they match the local-command rules first.
- Console commands require the console menu to be open.
- Console-command readiness and rejection messages appear as `[local]`.
- Successful console-command submission is recorded as `[console-submit]`.

## Local commands

- Local commands:
  - `cdbg.lookup <EditorID>`: exact Editor ID lookup.
  - `cdbg.lookup-prefix <prefix> [limit]`: bounded Editor ID prefix lookup.
    Default limit: `10`.
- Local commands become ready after `kDataLoaded`.
- Local command requests, responses, and readiness messages appear as `[local]`.
- `cdbg.` commands staying local is expected.
