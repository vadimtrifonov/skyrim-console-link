## Context

We need form-key-based exact inspection for runtime records that are not convenient to reach through Editor ID commands, especially vanilla leveled lists and mixed-source consumer paths.

## Decisions

1. `LocalCommandProcessor` routes two new local commands.
It keeps the existing readiness gate and dispatches:
- `cdbg.lookup-form`
- `cdbg.lookup-lvli-form`

2. One shared form-key parser owns `<plugin>|<local-formid>` validation.
The parser accepts one plugin token, one `|` separator, and one hexadecimal local form token. The parsed key is then used for exact local resolution.

3. `cdbg.lookup-form` is exact-form inspection only.
It resolves one exact form by plugin-qualified form key using the plugin-qualified local form ID and emits one result line for `miss` or one identifying hit line for a resolved form. The hit line includes at least the resolved runtime form ID, form type, and source plugin, and includes Editor ID and full name when available.

4. `cdbg.lookup-lvli-form` mirrors `cdbg.lookup-lvli` output policy.
It resolves one exact form by plugin-qualified form key using the plugin-qualified local form ID, requires the resolved form to be `LVLI`, emits a bounded summary plus direct-entry lines, preserves immediate-entry order, and treats nested `LVLI` references as one direct entry rather than expanding them recursively.

5. Result limiting stays local to `LookupLvliFormCommand`.
The built-in default is `10`. The command counts the full immediate-entry set first, then applies the output limit and reports truncation when entries were omitted.
