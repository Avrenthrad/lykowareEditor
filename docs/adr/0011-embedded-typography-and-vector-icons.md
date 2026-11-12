# ADR 0011: Embedded typography and original vector icons

Status: implemented locally, proposed for Joshua's review.

## Decision

Replace the desktop's default monospace face with unmodified Roboto Regular,
font name version 2.001047 (2015), pinned to googlefonts/roboto commit
38062f4b4a0be4346d07a928408da21602545e9e. The font's name table explicitly identifies
Apache License 2.0; preserve its embedded copyright and bundle the unchanged
upstream licence. Record source/version/checksum in third_party/README.md.
Do not substitute a later font under another licence without review.

Use an offline, desktop-only CMake wrapper that checks the SHA-256 and embeds
font bytes into the executable through a generated build header. Keep the static
bytes alive for the ImGui context lifetime and disable atlas ownership of them.
No source-tree file lookup, runtime download, font service or new dynamic library
is needed. Headless targets do not include the resource.

Use ten original vector icons on a 16-unit grid with consistent scaled strokes.
Render them beside text in native ImGui buttons and panel headings, preserving
keyboard behavior, command hints and stable widget IDs. Do not add an icon font,
copy artwork/code from restricted tools, or replace text labels with icons alone.

Apply the existing design-token approach more consistently: a 15px body baseline,
32px normal controls, token-scaled spacing, explicit neutral hover/active/border
colours and theme accents/focus. Keep layout and command behavior unchanged.

## Validation and limits

Existing workspace contrast checks now cover interactive surfaces as well as
backgrounds: 4.5:1 for normal/muted text in dark mode and 7:1 in high contrast.
Native input tests check that font metrics and control geometry still permit
palette forms, transaction rows, shortcut edits, docking and layout drafts.
X11 captures at 100% dark and 150% high contrast were inspected; the latter uses
same-instance MCP and launches from another working directory.

This is a first polish pass. Hardware 60fps, Windows, handheld input, full glyph
coverage, all small touch targets and screen-reader support remain unverified.
ImGui's lazy font atlas may perform initial glyph work on the UI thread; no new
performance guarantee is made. Future distributions must include bundled licences
and remain subject to Joshua's review. No artifacts or paid CI are uploaded.
