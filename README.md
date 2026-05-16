# Console Link

Console Link is a [CommonLibVR](https://github.com/alandtse/CommonLibVR) SKSE plugin for logging Skyrim console activity and submitting commands from an inbox file.

It is designed for external test loops: read `ConsoleLink.activity.log`, write commands to `ConsoleLink.inbox.txt`, and inspect the results. This is useful for LLM-assisted game testing, especially in Skyrim VR where in-game debugging workflows are limited.

The agent skill for operating Console Link is included at `.agents/skills/console-link-operation/`.

## Runtime support

| Runtime | Status |
| --- | --- |
| Skyrim VR `1.4.15` | Supported; extensively tested |
| Skyrim SE `1.5.97` | Supported |
| Skyrim AE `1.6.1170` | Supported; extensively tested |

## Requirements

- Skyrim VR: [SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/30457) + [VR Address Library for SKSEVR](https://www.nexusmods.com/skyrimspecialedition/mods/58101)
- Skyrim SE/AE: [SKSE](https://www.nexusmods.com/skyrimspecialedition/mods/30379) + [Address Library for SKSE Plugins](https://www.nexusmods.com/skyrimspecialedition/mods/32444)

## Activity log

`ConsoleLink.activity.log` is written to the SKSE logs folder under Documents and records the following event types:

| Prefix | Meaning |
| --- | --- |
| `[console-in]` | Console command entered in-game |
| `[console-out]` | Console output printed by the game |
| `[console-submit]` | Console command submitted from the inbox |
| `[local]` | Plugin-side events: local commands and responses, command readiness, and inbox status |

## Inbox

The default inbox file is `ConsoleLink.inbox.txt`, co-located with `ConsoleLink.activity.log`.

The inbox is a live command transport. On startup, Console Link clears existing inbox contents.

### Input modes

- **Append:** add newline-terminated command lines; each line is processed once.
- **Overwrite:** replace the file; all lines are processed as a new batch.

### Command types

- **Local:** `cdbg.*` lines, handled by the plugin instead of Skyrim's console.
- **Console:** all other lines, submitted only while the in-game console is open; otherwise rejected, not queued.

## Local commands

| Command | Result |
| --- | --- |
| `cdbg.lookup <EditorID>` | Exact Editor ID lookup |
| `cdbg.lookup-prefix <prefix> [limit]` | Editor ID prefix lookup; default limit `10` |
| `cdbg.lookup-form <plugin>\|<local-formid>` | Plugin-qualified local form lookup, for example `Skyrim.esm\|0003DF19` |
| `cdbg.lookup-lvli <EditorID> [limit]` | Direct leveled-list inspection by Editor ID; default limit `10` |
| `cdbg.lookup-lvli-form <plugin>\|<local-formid> [limit]` | Direct leveled-list inspection by plugin-qualified local form ID; default limit `10` |

## Configuration

```text
Data/SKSE/Plugins/ConsoleLink.ini
```

Options are documented in the INI.

## Developing

From the repository root:

```powershell
git submodule update --init --recursive
.\scripts\Test.ps1 -Mode releasedbg
.\scripts\Package.ps1 -Mode releasedbg
```

The package script writes a mod-manager-ready archive under `artifacts/`.

Deploy directly to a mod root:

```powershell
.\scripts\Deploy.ps1 -Target "C:\Path\To\ModOrganizer\mods\Console Link" -Mode releasedbg
```

`Deploy.ps1` creates `SKSE/Plugins/` under the target and copies the DLL and INI. Use `-OverwriteIni` to replace an existing INI.
