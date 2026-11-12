# ADR 0006: Schema-driven command forms

Status: Implemented locally; review pending

## Decision

Extend registry field metadata with titles, choices, defaults and numeric limits.
Discovery and MCP publish this same metadata. Include x-maxBytes for names/input
values because their backend limit counts UTF-8 bytes, whereas JSON Schema string
length counts Unicode code points. Command names offered for key binding come
from the registry rather than a separately maintained UI list.

Use a toolkit-independent CommandForm draft model to produce typed arguments.
It checks primitive types, required strings, numeric ranges, enum choices and
UTF-8 bounds without modifying document state. Unknown draft fields are rejected.
Draft validation is advisory: cross-field constraints, scene existence, history,
play state and atomic commit remain authoritative in the shared command service.
Only explicit submission creates a command request or undo record.

Render strings as text fields, integers as exact uint64 controls, booleans as
checkboxes and enum strings as dropdowns. Show field errors inline and disable
execution while draft/parse errors remain. Batch command arrays retain a clearly
labelled advanced JSON editor; nested schema widgets are not implemented yet.
Text/array widget drafts currently allow 16 KiB per field and reject oversized
defaults instead of silently truncating them. Headless protocol limits are unchanged.

Prefill workspace settings from the current snapshot and entity rename fields
from the selected entity. Rebound shortcuts execute parameterless commands directly;
commands needing parameters open their form. Ctrl+K focuses search, Escape goes
back/closes the palette and new search input returns to command results. These
are transient navigation/draft operations; committed edits still use the bus.

## Validation

Model tests cover defaults, enum/range/type rejection, malformed UTF-8, the
128-byte backend name limit, exact uint64 values, explicit shortcut unbinding,
unknown fields, and service dispatch without draft edits changing history.
The SDL interaction harness opens workspace.configure from search, toggles reduced
motion in the generated form, submits it and verifies the resulting scene snapshot.
The same flow is captured under X11 for visual inspection. Existing CLI/MCP,
workspace, history and persistence tests remain part of the desktop preset.
