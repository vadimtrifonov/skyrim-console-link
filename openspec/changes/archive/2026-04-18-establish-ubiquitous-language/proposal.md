## Why

The repo already has stable domain concepts, but the cross-cutting vocabulary still drifts across specs, code, logs, and tests. The same idea appears as `observation` or `capture`, and loaded runtime values appear as `settings` in specs but `config` in code and diagnostics.

## What Changes

- Amend existing specs, code, tests, and diagnostics to use one term per concept.
- Use `settings` for loaded runtime values.
- Use `observation` for passive console mirroring.
- Preserve runtime behavior, activity labels, `ConsoleLink.ini`, and current INI key names.

## Capabilities

### Modified Capabilities
- `plugin-settings`: startup settings language matches the loaded runtime values used by the plugin.
- `console-observation`: passive console mirroring uses `observation` rather than `capture`.

## Impact

- Will rename files, namespaces, types, tests, and diagnostics that still encode `config` or `capture` as internal domain terms.
