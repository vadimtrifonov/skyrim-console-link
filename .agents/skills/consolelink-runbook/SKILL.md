---
name: consolelink-runbook
description: Build, deploy, verify, and troubleshoot ConsoleLink in Skyrim VR. Use when agent needs the commands, likely file paths, or non-obvious runtime checks for this repo.
---

# ConsoleLink Runbook

## Commands

- Setup: `git submodule update --init --recursive`
- First `xmake` configure may need package/network access.
- Build: `.\scripts\Build.ps1 -Mode debug -IncludeTests`
- Test: `.\scripts\Test.ps1 -Mode debug`
- Package: `.\scripts\Package.ps1 -Mode debug`
- Deploy: `.\scripts\Deploy.ps1 -Target <MO2 mod path> -Mode debug`

## Paths

- Standard SKSE log dir: `ConsoleLink.log`, `ConsoleLink.console.log`, `ConsoleLink.outbox.txt`, `ConsoleLink.inbox.txt`
- If INI overrides are active, use startup logs as the source of truth for resolved inbox/outbox paths.

## Verify

- Check startup logs for resolved settings and path state.
- For command submission, check whether the console menu must be open.
- Check file-backed inputs are newline-terminated.

## Troubleshoot

- Build setup: run `git submodule update --init --recursive`.
- No behavior: startup warnings, rejected paths, failed runtime validation, wrong INI.
- Submission: local `cdbg.` handler consumed input; `ExecuteCommand` seam no longer matches; inbox replaced, truncated, or removed.
- Display: disabled in config; wrong resolved outbox path; watched file identity changed.

## Rules

- Deploy to Mod Organizer 2, not directly to Skyrim.
- Ask which MO2 instance to use if the target is ambiguous.
