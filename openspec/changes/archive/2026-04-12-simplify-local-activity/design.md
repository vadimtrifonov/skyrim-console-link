## Context

This change aligns the plugin-owned activity surface with its actual structure. It renames the internal local-activity taxonomy and structured activity log artifact, and removes the separate print surface entirely so local-command behavior is expressed only through activity records.

## Decisions

1. Rename `ConsoleActivityKind` in place.
Owner: `src/logging/`.
Change: rename `kLocalCommandActivity` to `kLocalActivity` while keeping the serialized wire channel as `[local]`.

2. Rename the structured activity log artifact at initialization.
Owner: `src/logging/ConsoleActivityLog.*`.
Change: write structured activity records to `ConsoleLink.activity.log` instead of `ConsoleLink.console.log`. Lifecycle logging remains `ConsoleLink.log`.

3. Remove print as one runtime slice.
Owners: `src/print/`, bootstrap/runtime wiring, config parsing, and tests/specs that only exist for print behavior.
Change: remove direct print, outbox configuration, outbox path resolution, outbox polling, outbox queue state, `console-print` activity, and print-specific output-capture suppression that only existed for plugin-authored console prints.

4. Local commands become activity-only.
Owners: local-command call sites and related specs/tests.
Change: keep local command request/response handling and `[local]` activity, but remove any immediate console echo path.
