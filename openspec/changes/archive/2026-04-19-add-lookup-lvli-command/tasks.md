## 1. Implemented

- [x] 1.1 Add `cdbg.lookup-lvli` local-command routing and exact Editor ID resolution for leveled-list inspection.
- [x] 1.2 Add bounded direct-entry dump formatting, including explicit miss, wrong-type, and truncation responses.

## 2. Verified

- [x] 2.1 Add automated coverage for argument validation, miss, wrong-type, direct-entry output, default and explicit limits, preserved entry order, and truncation reporting.
- [x] 2.2 Run the relevant automated test target and confirm the new command passes.

Evidence:
- `MSYSTEM= MINGW_PREFIX= MSYS2_PATH_TYPE= powershell.exe -NoProfile -ExecutionPolicy Bypass -File './scripts/Test.ps1' -Mode debug`

## 3. Deferred

No intentional carry-forward items.
