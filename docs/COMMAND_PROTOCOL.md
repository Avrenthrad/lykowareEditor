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

Command registrations are version 1. `commands.list` discovers input schemas,
including enum choices, defaults, labels and numeric bounds. x-maxBytes states
the UTF-8 byte limit where the backend uses bytes rather than Unicode length.
Names and mock input values accept 1–128 UTF-8 bytes. Unknown fields fail.

| Command | Arguments | Result data / effect |
| --- | --- | --- |
| `workspace.configure` | `scale_percent`: uint64, `theme`: string, `reduced_motion`: bool | Set scale (75–250), dark/high_contrast theme and reduced motion |
| `workspace.query` | none | Settings, panel positions, splits, keymap and selection |
| `workspace.save` | `path`: string | Export workspace TOML to a new file |
| `workspace.load` | `path`: string | Load settings/layout/keymap undoably; preserve scene and selection |
| `panel.dock` | `panel`: string, `dock`: string | Assign a registered panel to a dock slot or hidden |
| `layout.query` | none | Return authoritative split/tab layout |
| `layout.replace` | `layout`: object | Validate and replace the full split/tab tree undoably |
| `layout.reset` | none | Restore starter layout from panel/split presets |
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
| `editor.snapshot` | none | Coherent scene/workspace/selection/tokens/logs/history at the response revision |
| `history.query` | none | History cursor, undo/redo availability and named transaction entries |
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
Workspace configuration writes TOML schema v2 and reads v1/v2 (ADRs 0003/0007). Prefab/asset schemas and
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
resizable percentages. These are starter presets: panel.dock and layout.resize
reset any custom tree. The authoritative current tree is layout.query or
workspace.query.layout. UI state and scene edits share atomic transactions and
undo history.

Shortcut syntax is case-sensitive and canonical: modifiers Ctrl, Alt, Shift, Super
in that order, then A–Z, 0–9, F1–F24, Enter, Escape, Space, Tab, Backspace, Delete,
Home, End, PageUp, PageDown, Up, Down, Left or Right. Each shortcut has one command;
conflicts fail without changes. The default palette binding is Ctrl+K. Rebinding
updates palette results and desktop keyboard dispatch immediately.
Palette queries support empty strings and at most 128 UTF-8 bytes. Matching uses
ASCII case folding, with prefix/name/description ranking and stable name ordering.

Workspace TOML includes schema_version, scale_percent, theme, reduced_motion and
panels/splits/keymap tables. Version 2 adds layout and layout.nodes.ID tables;
version 1 loads into a generated starter tree. Only the generated subset plus
comments/whitespace is supported; unknown tables, keys, bindings, conflicting shortcuts and unsupported
versions fail before changing state. A workspace load preserves selection and the
scene; a scene load clears selection and preserves layout/preferences. Config
exports exclude selection and runtime data. File paths use explicit UTF-8 encoding;
embedded nulls are rejected. Actual Windows filesystem behavior remains untested.

## Desktop execution and panel snapshots

The C++ `CommandQueue` is an asynchronous adapter over the same service. Desktop
render/input code submits small requests, retains them until a ticket is accepted,
and polls completions without waiting for command execution. `queue_busy` and
`queue_full` mean nothing was accepted; retry only on a later frame. `queue_closed`,
`request_too_large`, `invalid_request` and `ticket_exhausted` are local submission
errors. Every accepted ticket receives one response while the queue remains alive.
Never replay an accepted mutation merely because its result has not arrived.

Capacity includes queued, running and completed-but-unpolled requests. Close
cancels undequeued requests with code `cancelled`; running requests finish. Shutdown
may therefore wait for a backend/file operation. Cancellation/unexpected queue
errors report revision zero as unknown; update cached revision from successful
snapshots, not error envelopes. There is no per-command interrupt yet.

`editor.snapshot` returns schema_version, mock, scene, workspace, selection,
tokens, logs and history together. All sections describe the response revision.
It excludes preview images and performs no file I/O. Request `preview.capture`
separately with expected_revision set to that snapshot revision. On stale_revision,
refresh the snapshot and present/re-evaluate the user's intent; do not blindly
retry edits. Cache panel data on the desktop thread and apply newer snapshots
as a unit rather than mixing individual panel replies from different revisions.

History contains cursor, can_undo, can_redo and entries. Entries retain original
commit revision, commands, mutation_count and applied status. Undo/redo changes
the cursor and current revision; it does not rewrite original commit revisions.
A failed edit preserves the redo branch; a changed edit after undo discards it.
No-op edits and queries create no entries. History stays in memory only.

## Split/tab layout contract

`layout.replace` takes `layout: {root, nodes, hidden}`. Each node is either
`{kind: "tabs", panels: [panel IDs], active: panel ID}` (active is empty for an empty leaf), or
`{kind: "split", axis: "horizontal"|"vertical", ratio: uint64, first: node ID, second: node ID}`.
Ratios are basis points, 500–9500. Input is bounded to 63 nodes and depth 8;
unknown fields, cycles, shared/unreachable nodes and invalid active tabs fail.
Every registered panel occurs exactly once across tabs and hidden. Node IDs use
1–64 ASCII letters/digits/underscore/hyphen. Discovery exposes the nested schema.

Validation precedes mutation. Normalization removes empty regions, collapses
single-child splits, sorts hidden IDs and assigns structural IDs root, root-0,
etc. These IDs are paths, not durable handles. An all-hidden layout has one empty
root. Equivalent replacements create no history record. Save/load includes tab
order, active tabs and hidden panels; arbitrary floating windows are unsupported.
Desktop gestures preview locally and submit one revision-guarded replacement;
rejection restores confirmed state. Other clients use the same commands.

## Desktop transaction drafts

The palette's transaction.execute form derives batchable choices from command
discovery (`mutation: true`, excluding transaction.execute itself). Each row uses
the same argument controls as a standalone command. Add/remove/reorder only edit
the draft. Execute submits the ordered commands array through the existing queue
with the current instance and expected revision. Child rows never execute alone.
The GUI limits batches to 64 rows; this is a local draft bound, not a new CLI/MCP
protocol limit. Queries, history operations, exports and recursive transactions
are excluded. Primitive validation is advisory; cross-row/domain validation and
atomic rollback belong to the shared service. Nested layout objects use typed layout controls plus explicit advanced JSON
import. A stale revision returns the existing structured service error.

## Shortcut discovery in the desktop

The modeless Shortcuts browser combines commands.list documentation with the
confirmed workspace keymap. It lists unbound commands and searches names,
descriptions and bindings with ASCII case folding. Edit prefills keymap.bind;
only explicit submission changes the workspace. Rebinding, unbinding, conflict
validation, revision checks and undo/redo use the existing shared service.

Action tooltips, context-menu shortcut labels, palette rows and the inspector
Undo hint use confirmed bindings, so undo/redo and externally edited keymaps
update them after snapshot refresh. Parameterized shortcuts open forms; contextual
Rename prefills the clicked entity, even when another entity is selected. Browser
search/navigation is transient UI state, with no separate document mutation path.

## Layout object drafts

The layout.replace palette field and batch rows expose split direction/share,
active tabs, tab order, panel grouping/hiding and split creation. Percentages map
to integer basis points without altering untouched values. Candidate edits run
through the existing graph validator and canonicalizer, in a private draft.
Execute still sends the same layout.replace object, with the same revision guard
and atomic history behavior; no service contract or persistence format changed.

Standalone desktop forms prefill the confirmed layout. New transaction rows use
the starter layout. Advanced JSON text edits block submission until explicitly
imported or discarded. Invalid imports and invalid typed candidates preserve the
valid draft; typed errors must be corrected or acknowledged before execution.
This widget supports the layout object contract specifically; other objects retain
advanced JSON controls. File, node, ratio and nesting limits remain unchanged.

## Form-local command feedback

The desktop tags local outgoing requests and pending queue tickets with a draft
generation. These tags are not sent to the command service or MCP. A completion
updates form feedback only when both generation and ticket match. Back, a new
search or opening another form clears local feedback; hiding the palette retains
the draft. Neither action cancels accepted work. Old replies still pass
through normal global status/snapshot handling.

A form cannot submit again while pending. Normal queue backpressure keeps an
unaccepted request waiting; local-buffer exhaustion or a terminal admission error
is shown and permits an explicit new submission. Errors preserve submitted arguments; editing them
clears obsolete feedback. Matching service fields are shown inline, with code,
field, request and response revision available in feedback details. Transaction
errors stay at batch level because the service currently does not return a row
index. Stale revisions request the existing snapshot refresh and require review
before explicit retry; mutations are never automatically replayed.

Successful query data is shown as a UTF-8-safe 4 KiB JSON preview, with a marker
when truncated. This is a presentation bound, not a wire-protocol change or a
modification of the original structured response. Feedback itself is transient
and creates no document/history mutation.

## Entity authoring actions

`entity.duplicate` requires `id` (existing unsigned entity ID) and `name`
(1–128 UTF-8 bytes). It allocates the next stable ID and selects the copy as one
transaction. `entity.delete` requires `id`; it removes that entity and clears
selection if selected. Both support expected_revision, batching and undo/redo.
Missing IDs return not_found/id. Exhausted allocation returns id_exhausted/id.
Deletion preserves next_id; failed batches roll back allocations and selection.
The current entity model contains only IDs/names; no runtime resources are cloned.
Scene JSON v1 is unchanged. New forms prefill the selected entity when available;
context actions target the clicked entity. Duplicate opens a name draft; Delete
executes directly and can be undone without a blocking confirmation dialog.

## Scene hierarchy

entity.reparent takes id and parent (unsigned IDs); parent 0 moves to scene root.
It validates existence, cycles and maximum depth 64 before committing, with full
transaction rollback and undo/redo. scene.tree returns schema_version 1 and a flat
parent-first nodes array containing id, name, parent and depth. Siblings sort by ID.
editor.snapshot includes the same tree as scene_tree; legacy scene.query remains
flat for compatibility. Duplicate keeps the source parent and does not copy its
children. Delete promotes direct children to root, retaining deeper relationships.
There are no transforms yet, so reparenting has no spatial semantics.

Scene files read versions 1 and 2. V1 means all entities are roots. Files with
parent links write v2 with a parent field on every entity (0 for roots); flat files
continue to write v1. Root-only v2 input therefore normalizes to v1 on export.
Invalid or excessive-depth hierarchy loads leave scene/history unchanged.

## Local authoring transforms

transform.set requires id and nine numeric components: px/py/pz (position),
rx/ry/rz (intrinsic XYZ Euler degrees), sx/sy/sz (signed scale). All values must
be finite; scale components must be nonzero. JSON integers/fractions convert to
IEEE-754 doubles for components; entity IDs remain exact unsigned integers.
transform.query takes id and returns the nine components, defaulting to identity.
Both require an existing entity. Sets participate in revisions/batches/undo.
Snapshots expose sparse nonidentity values in transforms, keyed by entity ID.

Duplicate copies the local transform; Delete removes it. Reparent and parent
removal preserve local values, without computing or preserving world transforms.
These values are authoring-only; the mock renderer/runtime does not interpret them.
Scene JSON v3 includes parent and transform on each entity. V1/v2 input supplies
identity transforms. Export uses v3 only when nonidentity transforms exist,
otherwise the previous v1/v2 rules apply. Invalid loads preserve scene/history.

## World transform query

transform.world takes id and returns a read-only world matrix plus convention
metadata. Matrices contain 16 row-major doubles and multiply column vectors.
Local composition is T*Rx*Ry*Rz*S (intrinsic XYZ Euler degrees); hierarchy
composition is parent_world*local. Coordinates are right-handed, Y up, -Z forward,
in editor units. Translation occupies indices 3, 7, 11. Sibling ordering is irrelevant.
The full affine matrix retains shear and reflection; no world TRS decomposition
or world-space mutation is implied. Reparent still preserves local values only.

Angles are reduced modulo 360 before conversion to radians. Nonfinite composition
intermediates return transform_range/transform; normal double rounding/underflow
still applies, so invertibility is not guaranteed. Queries do not alter revisions,
history or JSON v3 persistence. The mock preview does not consume these matrices.

## World-preserving reparent

entity.reparent_world takes id and parent (0 for root). It changes hierarchy and
local TRS atomically, preserving every affected subtree world-matrix component
within 1e-9 * max(1, abs(original component)). Existing entity.reparent continues
to preserve local values. Both share existence, cycle and depth validation.

The new operation computes inverse(new_parent_world)*old_world, decomposes to
local TRS and verifies recomposition plus the affected subtree. Reflection is
represented by a negative X scale when needed; equivalent Euler/scale values
may differ. Local shear, pivots/scales below 1e-12, nonfinite arithmetic, or failed
residual checks return cannot_preserve_world/parent without changing state/history.
This is a conservative operation, not a guarantee that all finite TRS cases pass.
Same-parent requests are no-ops. No matrix override or persistence change is added.

## Detached authoring prefabs

prefab.export takes id and path, captures that entity and all descendants, and
publishes a NEW file without modifying history. Root-local placement is retained,
not baked to world space; outside ancestors, siblings, runtime and workspace data
are excluded. Export-local IDs normalize to parent-first 1..N, with root 1.

Prefab JSON v1 has exactly type (lykoware.prefab), schema_version (1), root and
scene. The scene uses the existing v1/v2/v3 authoring payload; all entities must
belong to one rooted tree. Normalized export IDs are file-local, not durable override
handles. Imports share UTF-8/1 MiB file bounds, strict keys and graph validation.

prefab.instantiate takes path and parent (0 for scene root). It freezes validated
file content for the request, allocates fresh IDs, remaps parent links, copies local
transforms and selects the new root in one atomic transaction. Query selection
and scene.tree to inspect the new entities. Existing-parent/depth/allocation checks
run against the transaction candidate, so earlier batch commands can create the
parent and any later failure rolls everything back. Undo/redo includes selection,
relationships, transforms and allocation. Export is an external effect and is not
allowed inside transactions; instantiation is batchable.

Instances are detached copies: no linked updates, overrides, asset GUIDs, recursive
prefab references or runtime resources are introduced. Scene persistence is unchanged.
