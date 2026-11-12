# ADR 0009: Live command hints and shortcut discovery

Status: implemented locally, proposed for Joshua's review.

## Decision

Use registry descriptions and the confirmed workspace keymap as the sources for
read-only command presentation. Do not maintain a separate shortcut help list or
hard-code binding labels in panel instructions. A toolkit-independent CommandHints
model supplies deterministic name-ordered search across command name, description
and current shortcut, with ASCII case folding. Unbound commands remain discoverable.

Add a modeless Shortcuts browser. Its Edit action opens the existing keymap.bind
form with the command and confirmed shortcut prefilled. Draft editing does not
mutate state. Explicit submission uses the same queue, instance/revision checks,
conflict validation and undo history as CLI/MCP. Do not add a separate binding
handler or change the wire/persistence formats.

Use the same presentation model in action tooltips, entity context menus, palette
binding labels and inspector Undo hints. Registry documentation accompanies command
names and binding information. Parameterized shortcut hints explain that they open
forms. Contextual Rename prefills the clicked entity without changing selection.
History controls disable actions unavailable in the latest confirmed snapshot;
the service remains authoritative if state changes before execution.

Browser navigation/search is transient UI state. Target hardware, Windows and
screen-reader behavior remain unverified; this does not claim full accessibility.

## Validation

Model checks cover registry text, all/unbound commands, binding search, live
rebinding/unbinding and conflict rejection. SDL interaction opens the browser,
searches Undo, edits its binding to Ctrl+U, confirms the old key stops dispatching,
and verifies undo/redo restores bindings. Existing CLI/MCP/workspace tests remain
unchanged and verify the underlying command contract.
