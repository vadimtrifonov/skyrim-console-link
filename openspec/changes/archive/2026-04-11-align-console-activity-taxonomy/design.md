## Context

Current activity labels mix real console-seam events with plugin-owned activity:

- `console-out`, `console-in`, `console-submit`
- plugin-owned print and local activity labels

The proposal makes `console print` the canonical name for the plugin-authored print surface.

## Goals / Non-Goals

**Goals:**
- Reserve `console-*` channels for real console-path seams.
- Adopt `console-print` for plugin-authored print activity.
- Adopt `local` for plugin-local request and response activity.
- Use `print` as the canonical config surface for plugin-authored print lines.

**Non-Goals:** Not specified by the proposal.

## Decisions

### Canonical channels

- `console-out`: passive output observed from the validated console output seam
- `console-in`: accepted input observed from the validated `ExecuteCommand` seam
- `console-submit`: plugin-submitted commands that traverse the validated engine submission seam
- `console-print`: plugin-authored lines printed into the in-game console
- `local`: plugin-local request and response activity that never traverses an engine console seam

### Canonical config names

- `EnableConsolePrint`
- `[Print]`

### Implementation shape

Centralize the taxonomy in one pure helper that maps activity kinds to canonical channel strings and builds the formatted record text.

## Risks / Trade-offs

- Renaming the config surface to `print` is a breaking config change.

## Migration Plan

- Ship `EnableConsolePrint` and `[Print]` in examples.

## Open Questions

None.
