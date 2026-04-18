## Decisions

### Canonical terms

- `settings`: loaded runtime values used after parsing
- `observation`: passive console input or output mirroring from validated engine seams

### Module alignment

- Move loaded runtime settings code into `src/settings` / `Settings::`.
- Rename the loaded settings type so call sites read `Settings::Values`.
- Move passive console mirroring code from `src/capture` / `Capture::` to `src/observation` / `Observation::`.
- Align lifecycle diagnostics and test names with the same terms.

