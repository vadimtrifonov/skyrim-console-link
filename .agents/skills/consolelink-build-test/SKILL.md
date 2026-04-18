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

- If `xmake` selects `mingw` or cannot find `cc`/`cxx`, run from PowerShell or the VS dev shell, not Git Bash/MSYS.
- Clear `MSYSTEM`, `MINGW_PREFIX`, and `MSYS2_PATH_TYPE` if needed before rerunning build or test.
