## Why

The current activity naming still mixes real console-path events with plugin-owned local activity, which makes logs and specs harder to reason about than they need to be. This should be cleaned up before additional local command surface area is added.

## What Changes

- Rename the current display surface from "console display" to "console print" where that better matches the actual execution path.
- Reserve `console-*` activity channels for events that traverse a real console-path seam.
- Reclassify plugin-owned local command request and response activity under a local channel instead of a console channel.
- Align specs, logs, and shipped terminology around the chosen activity taxonomy.

## Capabilities

### New Capabilities

### Modified Capabilities
- `plugin-settings`: The canonical config surface for plugin-authored print lines is renamed from `display` to `print`.
- `console-observation`: Activity naming for passive input/output observation is aligned with the final taxonomy.
- `console-print`: The print surface and its canonical activity naming are aligned with the real execution path.
- `command-submission`: Submitted command records are distinguished cleanly from local-only outcomes.
- `local-commands`: Local command request and response records move to the non-console local taxonomy.

## Impact

- Affects console activity record names in logs, specs, and any code that formats or routes activity labels.
- May rename one or more capability specs to match the final terminology.
- Requires careful migration to avoid breaking assumptions in tests, tooling, and runtime diagnostics.
