## Motivation

The inbox works for append-only writers, but overwrite/truncate writers can be polled mid-rewrite. In live use that turned one intended command into fragments such as `ehuntress`, `4`, or `d 1`.

## Proposed change

After inbox readiness, detect whole-file rewrite cases:

- file identity changed
- file size shrank below the tracked cursor
- file was removed and recreated

When a rewrite is detected, treat the file contents as one input batch instead of append growth. Accept the batch only after it is stable. Queue its non-empty lines; the final line does not need a trailing newline. Do not replay unchanged contents on later polls. A later rewrite is still new input.

Reorganize the inbox requirements around four responsibilities:

1. startup inbox clear and readiness/failure announcement
2. newline-terminated append input after inbox readiness
3. stable rewritten/recreated file input after inbox readiness
4. routing accepted inbox lines to local or console command processors

Use these terms in specs and touched code/tests:

- `startup inbox clear`: the one-time startup create/truncate operation
- `inbox readiness`: the boundary after the inbox is known empty and ready for new input
- `after inbox readiness`: preferred over `post-baseline` / `post-startup`
- `accepted inbox line`: a line that passed inbox ingestion and is ready to route

### Modified Capabilities

- `inbox`: support append writers and whole-file rewrite writers safely, using the clearer inbox lifecycle/ingestion/routing vocabulary.

## Implementation sketch

- Rename startup-clear code to match the lifecycle term.
   - `ZeroInbox.*` -> `StartupInboxClear.*`
   - `EstablishZeroInboxFile` -> `ClearInboxAtStartup`
   - `DescribeZeroInboxResult` -> `DescribeStartupInboxClearResult`
   - `InputProcessor::EstablishZeroInbox` -> `EstablishInboxReadiness`

- `InboxFileReader` owns inbox file open, observe, and range-read I/O.

- `FileObservation`: identity, size, last-write time for an existing inbox file. `InboxState` uses it to choose append vs rewrite and to compare the inbox path before and after reading a rewrite.

- Enter rewrite mode when the append cursor no longer describes the file: identity changed, size is below the cursor, or the file reappeared after being missing. Discard buffered and queued input from the previous file contents, then read from offset `0`.

- Do not trust a whole-file read if the watched path changed while it was being read.
   - Observe the inbox path, read the file from offset `0`, then observe the inbox path again.
   - If the observations differ, queue nothing and try again on a later poll.
   - If they match, accept the read:
     - queue non-empty lines from the stable file, including a final line without a trailing newline
     - set the cursor to the stable size
     - close the rewrite event

- After accepting a rewrite, resume cursor tracking at the accepted file size. Repeated polls of that same file state do not replay it; a later replacement, truncation, or remove/recreate starts a new rewrite.
