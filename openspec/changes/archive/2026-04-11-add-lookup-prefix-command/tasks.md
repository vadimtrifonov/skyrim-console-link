## 1. Implemented

- [x] 1.1 Add `cdbg.lookup-prefix` local-command routing and bounded prefix lookup execution over the native Editor ID map.
- [x] 1.2 Keep exact lookup behavior unchanged while adding ordered multi-result formatting, default-limit handling, and truncation reporting for prefix lookup.

## 2. Verified

- [x] 2.1 Add automated coverage for prefix lookup argument validation, default and explicit limits, ordered output, and truncation reporting.
- [x] 2.2 Run the relevant automated test target and confirm the lookup command change passes.

Evidence: `.\scripts\Test.ps1 -Mode debug` passed.

## 3. Deferred

No intentional carry-forward items.
