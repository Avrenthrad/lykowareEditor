# Command protocol v1

Every CLI/MCP operation goes through `CommandService`, which owns a serialized
session around `CommandBus`. The registry owns command names, argument fields,
documentation and typed mutation decoding. No frontend writes document state.
The service serializes queries with mutations so response state is consistent.

## Request and result

Requests require `instance` and `command`. `arguments` defaults to `{}`.
`expected_revision` is optional; provide it for optimistic concurrency. A missing
or wrong instance fails before execution. A stale revision fails without mutation.
Revision increases once for a changed transaction, undo or redo. No-op mutations,
queries and saves preserve the revision. Error results do not alter history.
Malformed JSON errors have revision 0 because parsing precedes session dispatch;
query `scene.query` for the current revision rather than treating it as a reset.

Command registrations are version 1. `commands.list` discovers input schemas.
Names and mock input values accept 1–128 UTF-8 bytes. Unknown fields fail.

| Command | Arguments | Result data / effect |
| --- | --- | --- |
| `workspace.configure` | `scale_percent`: uint64, `theme`: string, `reduced_motion`: bool | Set scale (75–250), dark/high_contrast theme and reduced motion |
| `workspace.query` | none | Settings, panel positions, splits, keymap and selection |
| `workspace.save` | `path`: string | Export workspace TOML to a new file |
| `workspace.load` | `path`: string | Load settings/layout/keymap undoably; preserve scene and selection |
| `panel.dock` | `panel`: string, `dock`: string | Assign a registered panel to a dock slot or hidden |
| `layout.resize` | `dock`: string, `percent`: uint64 | Resize left/right/bottom split (10–50); retain 30% center width |
| `keymap.bind` | `command`: string, `shortcut`: string | Bind a canonical shortcut; empty string unbinds |
| `selection.set` | `id`: uint64 | Select an entity; zero clears selection |
| `selection.query` | none | Selected `entity` with id/name, or null |
| `design.tokens` | none | Central theme/scale/spacing/type/motion/geometry tokens |
| `palette.search` | `query`: string | All matching command descriptors, schemas and current shortcuts |
| `commands.list` | none | Array of command registrations and schemas |
| `entity.create` | `name`: string | Create stable entity ID; query scene for IDs |
| `entity.rename` | `id`: uint64, `name`: string | Rename existing entity |
| `play.start` | none | Start simulated play |
| `play.stop` | none | Stop simulated play |
| `play.input` | `value`: string | Record input while playing |
| `history.undo` | none | Undo one complete transaction |
| `history.redo` | none | Redo one complete transaction |
| `transaction.execute` | `commands`: array | Apply mutations as one transaction |
| `scene.query` | none | Versioned scene snapshot plus mock runtime state |
| `scene.load` | `path`: string | Validate authoring JSON and replace scene undoably |
| `scene.save` | `path`: string | Save authoring JSON to a new file; returns `saved` |
| `logs.query` | none | `mock: true`, `messages`: latest mock log strings |
| `preview.capture` | none | `mock`, `width`, `height`, `mimeType`, `ppm` |

Mutation successes without query data return `{}`. Errors return stable codes:
`instance_mismatch`, `stale_revision`, `invalid_request`, `invalid_name`,
`not_found`, `not_playing`, `invalid_input`, `input_limit`, `id_exhausted`,
`empty_transaction`, `invalid_scene`, `history_empty`, `backend_failure`,
`operation_failed`, `invalid_workspace`, `invalid_scale`, `invalid_theme`,
`unknown_panel`, `invalid_dock`, `invalid_split`, `invalid_shortcut`,
`shortcut_conflict`, or transport `parse_error`. Messages describe the failure.

Example atomic batch:

```json
{"instance":"demo","command":"transaction.execute","expected_revision":0,"arguments":{"commands":[{"command":"entity.create","arguments":{"name":"Player"}},{"command":"play.start"},{"command":"play.input","arguments":{"value":"right"}}]}}
```

Only mutations may be nested: query, save, history and recursive transaction
commands fail validation. The entire batch is validated against candidate state;
a failed command leaves the document, runtime and undo/redo branches unchanged.
Bridge implementations must be pure transformations: external side effects
cannot participate until an explicit commit protocol is designed.

## Scene files

Schema v1 contains exactly `schema_version`, `next_id` and `entities`. Each entity
has exactly `id` and `name`; IDs are unique, positive and less than `next_id`.
Runtime play state, logs and inputs are not authoring data and are not saved.
Unknown versions, duplicate keys/IDs, invalid names and oversized files fail
before replacement. Loading is undoable, including restoration of prior runtime.

Save validates and writes a complete staging file beside the destination, then
publishes it with a hard link that fails if the destination exists. It does not
overwrite existing files. Hard-link support is required; failure does not fall
back to a partial copy. Save is an explicit external export: undo cannot retract
a published file. This provides atomic publication, not power-loss durability.
Workspace configuration is TOML schema v1 (ADR 0003). Prefab/asset schemas and
glTF import remain future work.

## MCP over stdio

Start `lyko --instance demo --mcp`. The caller owns the process; stdout contains
only newline-delimited JSON-RPC messages. No listening socket is opened. Send
`initialize`, then `notifications/initialized`, followed by `tools/list` or
`tools/call`. Supported protocol versions are 2024-11-05 and 2025-06-18.

Each registered command becomes a tool with an input schema containing `instance`,
optional `expected_revision` and nested command `arguments`:

```json
{"jsonrpc":"2.0","id":1,"method":"tools/call","params":{"name":"entity.create","arguments":{"instance":"demo","expected_revision":0,"arguments":{"name":"Player"}}}}
```

Results include JSON text content and `isError`. The 2025-06-18 protocol also
returns the same envelope as `structuredContent`. Command failures are tool errors;
malformed JSON-RPC/lifecycle/method failures use JSON-RPC errors. Notifications
never execute tools. Different instances use different processes and histories;
there is no global router or shared scene persistence. External MCP-client
interoperability testing is still required.

## Workspace, palette and shortcuts

Panel IDs are `scene`, `inspector`, `viewport`, `console`, `history`. Dock slots
are `left`, `right`, `center`, `bottom`, `hidden`; only left/right/bottom have
resizable percentages. This starter model does not yet represent arbitrary nested
docking. UI state and scene edits share atomic transactions and undo history.

Shortcut syntax is case-sensitive and canonical: modifiers Ctrl, Alt, Shift, Super
in that order, then A–Z, 0–9, F1–F24, Enter, Escape, Space, Tab, Backspace, Delete,
Home, End, PageUp, PageDown, Up, Down, Left or Right. Each shortcut has one command;
conflicts fail without changes. The default palette binding is Ctrl+K. Rebinding
updates palette results immediately; actual OS keyboard dispatch awaits the GUI.
Palette queries support empty strings and at most 128 UTF-8 bytes. Matching uses
ASCII case folding, with prefix/name/description ranking and stable name ordering.

Workspace TOML includes schema_version, scale_percent, theme, reduced_motion and
panels/splits/keymap tables. Only the generated v1 subset plus comments/whitespace
is supported; unknown tables, keys, bindings, conflicting shortcuts and unsupported
versions fail before changing state. A workspace load preserves selection and the
scene; a scene load clears selection and preserves layout/preferences. Config
exports exclude selection and runtime data. File paths use explicit UTF-8 encoding;
embedded nulls are rejected. Actual Windows filesystem behavior remains untested.
