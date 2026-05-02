---
name: consolelink-build-test
description: Build, test, and verify ConsoleLink locally. Troubleshoot toolchain failures.
---

# ConsoleLink Build/Test

## Setup

- Initialize submodules: `git submodule update --init --recursive`

## Commands

- Build: `./scripts/Build.ps1 -Mode debug`
- Test: `./scripts/Test.ps1 -Mode debug`
- Format: `./scripts/Format.ps1`
- Verify: `./scripts/Verify.ps1`

## Preference

- Prefer `Verify` for complete verification (includes static analysis)
- Use `Build`, `Test`, `Format` only when you specifically need that narrower step.

## Troubleshooting

- If `xmake` incorrectly selects `mingw` or cannot find `cc`/`cxx`, rerun from PowerShell or the VS dev shell instead of Git Bash/MSYS.
- If the wrong toolchain is still inherited, clear `MSYSTEM`, `MINGW_PREFIX`, and `MSYS2_PATH_TYPE` inside PowerShell before rerunning build, test, or verify.
