## Context

Lookup output is formatted in two leaf modules:
- `src/local_commands/LookupCommand.cpp` builds exact-hit lines
- `src/local_commands/LookupPrefixCommand.cpp` builds prefix-match lines

Both already format form ID, form type, and optional name locally, and they already duplicate similar formatting helpers.

## Decisions

1. Resolve source from `RE::TESForm`.
Implementation seam: use `RE::TESForm::GetFile()` to obtain the form source file and `RE::TESFile::GetFilename()` to obtain the emitted plugin string.

2. Patch both lookup formatters in place.
Write scope: append ` source="<plugin>"` in the exact-hit formatter in `LookupCommand.cpp` and in the prefix-match formatter in `LookupPrefixCommand.cpp`.

3. Do not introduce a shared helper as part of this change.
Reason: the two modules already keep similar formatting code local, and this change only adds one field at each call site.
