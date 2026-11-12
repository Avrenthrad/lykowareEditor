# ADR 0003: Command-driven workspace before the desktop adapter

Status: Implemented locally; review pending

## Context

The desktop shell must share behavior with CLI/MCP and keep accessibility settings
and layouts independent from scenes. Network access currently blocks acquisition
of the selected Dear ImGui/SDL3 dependencies. A window cannot be validated yet.

## Decision

Implement a toolkit-independent Workspace value under `editor/src/ui/`, managed
by the same atomic transactions as scene edits. It owns theme, UI scaling,
reduced-motion preference, starter panel positions/splits, keymap and selection.
Theme/scale/layout/keymap mutations are undoable. Queries expose workspace state,
selected entity, design tokens and searchable command descriptors.

The starter layout assigns scene, inspector, viewport, console and history panels
to left/right/center/bottom slots or hidden. Split constraints retain center space.
This is a model for the first desktop layout, not an implementation of arbitrary
nested docking. The Dear ImGui adapter will provide the actual window, input,
font rendering and docking and translate every interaction to commands.

Use central dark/high-contrast colour, spacing, typography, geometry and motion
tokens. Scale logical sizes by 75–250 percent, retain a separate 44px base touch
hit target and support zero-duration reduced motion. Contrast tests cover text
and muted text on both surfaces (4.5:1 dark; 7:1 high contrast). These checks do
not establish complete accessibility, DPI quality or real-device usability.

The command palette searches all command registrations. Prefix name matches rank
before name substrings, then description matches; ties sort by name. ASCII search
is case-insensitive; Unicode case folding is deferred. Every result includes
current shortcuts and input schemas. Commands requiring parameters still require
a frontend to collect or supply them before invocation. Canonical shortcut syntax
uses optional Ctrl, Alt, Shift, Super modifiers in that order plus a supported key.
Unbind a shortcut before assigning it to another command; conflicts are errors.

Persist workspace configuration as TOML schema v1, separately from scene JSON.
The writer emits a strict, independently parsed TOML subset (basic quoted strings,
booleans, unsigned decimal integers and explicit panels/splits/keymap tables).
The loader accepts this generated format plus comments and whitespace. It is not
a general TOML parser: unsupported syntax and unknown/duplicate fields fail.
Selection is never persisted. Workspace load preserves scene/runtime/selection;
scene load preserves workspace preferences/layout and clears selection. Undo
restores the complete prior state in either case.

Share bounded UTF-8-aware file handling and atomic no-overwrite publication with
scene persistence. Saves are explicit exports outside undo history. Windows and
GUI integration remain unverified; no new dependency or engine access is required.

## Consequences

CLI and MCP can exercise core shell workflows now. A future desktop frontend can
consume tested state and commands instead of introducing UI-only behavior. Actual
docking, icon/font selection, parameter widgets, input dispatch, focus handling,
screen-reader support and frame-rate measurements remain separate work.
