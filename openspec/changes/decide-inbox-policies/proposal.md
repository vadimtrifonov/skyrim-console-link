## Why

The inbox implementation is stable enough that the remaining edge policies should be made explicit instead of being left to incidental behavior. These decisions affect restart semantics, clear/reset behavior, and whether queue limits stay fixed or become configurable.

## What Changes

- Decide the replay or acknowledgement policy for consumed inbox lines across restarts.
- Decide whether inbox clear, replace, or truncation should cancel commands already lifted onto the game-thread submission path.
- Decide whether queue caps remain fixed implementation policy or become user-configurable settings.
- Document the chosen policies in specs and align implementation with them.

## Capabilities

### New Capabilities

### Modified Capabilities
- `command-submission`: Inbox replay, reset, cancellation, and queue-cap behavior become explicit product policy.
- `plugin-settings`: Queue-cap settings may change depending on whether configurability is adopted.

## Impact

- Affects command-submission state handling, restart behavior, and possibly shipped INI surface area.
- Requires test updates for whichever replay, cancellation, and configurability policies are chosen.
