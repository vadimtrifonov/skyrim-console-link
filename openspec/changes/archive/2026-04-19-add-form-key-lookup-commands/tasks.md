## 1. Implemented

- [x] 1.1 Add `cdbg.lookup-form` local-command routing and exact plugin-qualified form-key resolution.
- [x] 1.2 Add `cdbg.lookup-lvli-form` local-command routing and exact plugin-qualified `LVLI` inspection with bounded direct-entry output.
- [x] 1.3 Add one shared `<plugin>|<local-formid>` parser for the new form-key commands.

## 2. Verified

- [x] 2.1 Add automated coverage for malformed form-key input, miss, exact-form hit output, `LVLI` wrong-type, bounded direct-entry output, preserved entry order, truncation reporting, and space-containing plugin filenames.
- [x] 2.2 Run the relevant automated test target and confirm the new commands pass.

Evidence:
- `MSYSTEM= MINGW_PREFIX= MSYS2_PATH_TYPE= powershell.exe -NoProfile -ExecutionPolicy Bypass -File './scripts/Test.ps1' -Mode debug`

## 3. Deferred

No intentional carry-forward items.
