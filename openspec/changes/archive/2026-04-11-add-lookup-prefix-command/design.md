## Context

Add `cdbg.lookup-prefix <prefix> [limit]` on the existing local-command path. The command does bounded Editor ID prefix search over the native Editor ID map, uses built-in default limit `10` when `limit` is omitted, and reports truncation explicitly.

## Decisions

1. `LocalCommandDispatcher` stays the routing boundary.
It recognizes `cdbg.lookup-prefix`, keeps the existing `kDataLoaded` deferral behavior, and does not forward the command to engine submission.

2. Prefix-search execution lives in `LookupPrefixCommand`.
It parses `prefix` and optional `limit`, performs the native map search, and returns response lines. `LookupCommand` remains exact lookup only. Logging and printing remain outside both command modules.

3. The default result limit is built in.
When `limit` is omitted, the command uses default limit `10`.

4. Requested `limit` must be `>= 1`.
`0` and non-numeric `limit` values are invalid command usage.

5. Output must be deterministic before truncation is applied.
Collect matches, order them by Editor ID, then emit up to the effective limit.

6. Truncation is explicit user-visible output.
If more matches exist than are emitted, the response says the result was truncated.
