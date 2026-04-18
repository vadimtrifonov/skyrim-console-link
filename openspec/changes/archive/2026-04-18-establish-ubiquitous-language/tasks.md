## 1. Implemented

- [x] 1.1 Amend the `plugin-settings` and `console-observation` specs to use the canonical terminology.
- [x] 1.2 Rename settings-facing code to use `settings` terminology.
- [x] 1.3 Rename passive console mirroring code from `capture` to `observation` and align related diagnostics and tests.
- [x] 1.4 Sweep remaining code and tests for deprecated internal aliases.

## 2. Verified

- [x] 2.1 Build ConsoleLink and record the command and result.
  - Command: `powershell -NoProfile -ExecutionPolicy Bypass -Command "Remove-Item Env:MSYSTEM -ErrorAction SilentlyContinue; Remove-Item Env:MINGW_PREFIX -ErrorAction SilentlyContinue; Remove-Item Env:MSYS2_PATH_TYPE -ErrorAction SilentlyContinue; & ./scripts/Build.ps1 -Mode debug"`
  - Result: success; `xmake` built `ConsoleLink.dll` in `build/windows/x64/debug`.
- [x] 2.2 Run the relevant automated tests and record the command and result.
  - Command: `powershell -NoProfile -ExecutionPolicy Bypass -Command "Remove-Item Env:MSYSTEM -ErrorAction SilentlyContinue; Remove-Item Env:MINGW_PREFIX -ErrorAction SilentlyContinue; Remove-Item Env:MSYS2_PATH_TYPE -ErrorAction SilentlyContinue; & ./scripts/Test.ps1 -Mode debug"`
  - Result: success; `ConsoleLinkTests.exe` built and reported `All tests passed.`
- [x] 2.3 Grep for remaining internal legacy aliases and record the result.
  - Command: `rg -n "\b(config|Config|capture|Capture)\b" src tests xmake.lua`
  - Result: no matches.
