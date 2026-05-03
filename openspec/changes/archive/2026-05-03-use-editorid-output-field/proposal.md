## Motivation

Local-command output uses `editor="..."` for Editor IDs, which is easy to misread as an editor/tool reference rather than the form's EditorID.

## Proposed change

Render Editor ID fields as `editorID="..."` in local-command responses.

### Modified Capabilities

- `local-command-lookup-form`: hit output uses `editorID="..."`, including `editorID=""`.
- `local-command-lookup-prefix`: match output uses `editorID="..."`.
- `local-command-lookup-lvli`: entry output uses `editorID="..."`.
- `local-command-lookup-lvli-form`: resolved hit, wrong-type, and entry output uses `editorID="..."`, including `editorID=""` on resolved target lines.
