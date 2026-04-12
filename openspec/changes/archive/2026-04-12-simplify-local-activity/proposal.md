## Why

The current structured activity surface still mixes plugin-local logging with local command execution terminology, and its filename still understates what the log actually contains. The separate print surface also adds configuration, runtime wiring, and output-capture complexity that should be removed so local commands produce only `[local]` activity.

## What Changes

- **BREAKING** Rename the local activity taxonomy and the structured activity log artifact to match their actual scope.
- **BREAKING** Remove print entirely, including direct print, outbox support, print settings, and `console-print` activity.
- **BREAKING** Do not preserve backward compatibility for removed print settings or outbox workflows.

## Capabilities

### Modified Capabilities
- `plugin-logging`: Rename the structured activity log artifact.
- `plugin-settings`: Remove print-specific and outbox-specific configuration with no compatibility shim.
- `local-commands`: Remove direct local-response printing so local commands produce only `[local]` activity.
- `console-print`: Remove the print capability from the product contract.
