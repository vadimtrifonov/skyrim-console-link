## Why

The repo already has a real domain vocabulary, but the most important cross-cutting terms are still inconsistent across specs, code, logs, tests, and task notes. The result is a language drift where the same concept appears as `observation` or `capture`, `display` or `print`, `settings` or `config`, and local-only activity is still mixed into the `console-*` taxonomy. A dedicated change is needed so ubiquitous language becomes an explicit product asset rather than an incidental byproduct of implementation.

## What Changes

- Define a canonical ubiquitous-language glossary for the repo and use it as the source of truth for specs, code-facing terminology, logs, and task notes.
- Standardize the primary capability names and activity labels so each domain concept has one canonical term.
- Decide whether ambiguous terms such as `bridge` remain first-class domain concepts or are removed in favor of clearer canonical wording.
- Align related specs and implementation-facing terminology with the chosen glossary without widening runtime behavior.

## Capabilities

### New Capabilities

### Modified Capabilities
- `plugin-logging`: logging terminology aligns around the canonical names for lifecycle logs, activity logs, and related startup behavior.
- `plugin-settings`: startup configuration language aligns around the canonical settings vocabulary.
- `console-observation`: observational terminology is aligned with the chosen canonical term and activity labels.
- `console-display`: the print/display surface and its activity naming are aligned with the canonical glossary.
- `command-submission`: inbox, submission, local-command routing, and local/non-local outcomes use the canonical terms.
- `local-commands`: local command terminology and local activity labels align with the canonical glossary.

## Impact

- Affects spec names, requirement wording, canonical activity labels, and code-facing terminology that currently drift across modules.
- May rename one or more capabilities, activity channels, log messages, or implementation types to preserve one term per concept.
- Should produce a durable glossary and naming rules that future changes can reference instead of re-deciding terminology piecemeal.
