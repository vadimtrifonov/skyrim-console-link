## Unit tests

Tests added/updated in `tests/InboxStateTests.cpp`:

- `TestMissingAfterReadinessMakesRecreatedFileRewriteInput`
- `TestEmptyRewriteWaitsForLaterContents`
- `TestRewriteAcceptsFinalLineWithoutTrailingNewline`
- `TestAppendAfterRewriteStartsAtAcceptedRewriteSize`
- `TestAcceptedPrefixMismatchStartsRewriteForSameIdentityGrowth`
- `TestSameSizeOverwriteStartsRewriteWhenWriteTimeChanges`
- `TestQueueFullStillDetectsSameIdentityOverwriteGrowth`
- `TestAcceptedRewriteIsNotReplayedUntilAnotherRewrite`

## Runtime tests

Runtime: Skyrim VR / Wabbajack - Sovngarde 1.3.

1. **PASS: append**
   - Input: append `cdbg.lookup-prefix CL_APPEND_20260503_193306 1\n`
   - Observed:
     - `[local] cdbg.lookup-prefix CL_APPEND_20260503_193306 1`
     - `[local] lookup-prefix "CL_APPEND_20260503_193306" -> miss`

2. **PASS: shorter same-file overwrite/truncate**
   - Input: overwrite with `cdbg.lookup-prefix CL_SHORT_20260503_193306 1\n`
   - Observed:
     - `ConsoleLink.log`: `Inbox '...' was truncated; reset file tracking for the rewritten inbox contents`
     - `ConsoleLink.activity.log`: `[local] cdbg.lookup-prefix CL_SHORT_20260503_193306 1`

3. **PASS: larger same-file overwrite**
   - Input: overwrite same file with `cdbg.lookup-prefix CL_LONG_OVERWRITE_20260503_193306_EXTRA_LONG_MARKER 1\n`
   - Observed:
     - `ConsoleLink.log`: `Inbox '...' was overwritten; reset file tracking for the rewritten inbox contents`
     - `ConsoleLink.activity.log`: `[local] cdbg.lookup-prefix CL_LONG_OVERWRITE_20260503_193306_EXTRA_LONG_MARKER 1`
   - Confirms: the accepted command begins at `cdbg.lookup-prefix`, not a suffix of the overwritten file.

4. **PASS: delete then recreate**
   - Input: remove inbox, then recreate with `cdbg.lookup-prefix CL_RECREATE_20260503_193306 1\n`
   - Observed:
     - `ConsoleLink.log`: `Inbox '...' was removed; reset file tracking for the rewritten inbox contents`
     - `ConsoleLink.activity.log`: `[local] cdbg.lookup-prefix CL_RECREATE_20260503_193306 1`

5. **PASS: empty rewrite then later content**
   - Input: overwrite with empty contents, then overwrite with `cdbg.lookup-prefix CL_EMPTY_THEN_CONTENT_20260503_193306 1\n`
   - Observed:
     - `[local] cdbg.lookup-prefix CL_EMPTY_THEN_CONTENT_20260503_193306 1`
     - `[local] lookup-prefix "CL_EMPTY_THEN_CONTENT_20260503_193306" -> miss`

6. **PASS: final line without trailing newline**
   - Input: overwrite with `cdbg.lookup-prefix CL_NO_NEWLINE_20260503_193306 1`
   - Observed:
     - `[local] cdbg.lookup-prefix CL_NO_NEWLINE_20260503_193306 1`
     - `[local] lookup-prefix "CL_NO_NEWLINE_20260503_193306" -> miss`

7. **PASS: append after accepted rewrite**
   - Prior state: test 6 accepted rewritten contents `cdbg.lookup-prefix CL_NO_NEWLINE_20260503_193306 1`.
   - Input: append `cdbg.lookup-prefix CL_APPEND_AFTER_REWRITE_20260503_193306 1\n`
   - Observed:
     - `[local] cdbg.lookup-prefix CL_APPEND_AFTER_REWRITE_20260503_193306 1`
     - `[local] lookup-prefix "CL_APPEND_AFTER_REWRITE_20260503_193306" -> miss`
   - Confirms: the append was read after the accepted rewrite contents; `CL_NO_NEWLINE_20260503_193306` was not replayed.

8. **PASS: atomic replacement**
   - Input: move temp file over inbox with `cdbg.lookup-prefix CL_REPLACE_MOVE_20260503_193306 1\n`
   - Observed:
     - `ConsoleLink.log`: `Inbox '...' was atomically replaced; reset file tracking for the rewritten inbox contents`
     - `ConsoleLink.activity.log`: `[local] cdbg.lookup-prefix CL_REPLACE_MOVE_20260503_193306 1`

9. **PASS: same-size same-file overwrite**
   - Input: overwrite `cdbg.lookup-prefix CL_SAMESIZE_A_20260503_1936 1\n` with same-length `cdbg.lookup-prefix CL_SAMESIZE_B_20260503_1936 1\n`
   - Observed:
     - `ConsoleLink.log`: `Inbox '...' was overwritten; reset file tracking for the rewritten inbox contents`
     - `ConsoleLink.activity.log`: `[local] cdbg.lookup-prefix CL_SAMESIZE_B_20260503_1936 1`
