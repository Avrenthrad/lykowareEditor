# Standalone editor foundation

The editor currently runs headlessly with a deterministic mock backend. It has no
engine dependency, external libraries, display/GPU requirement or network listener.
An optional ImGui/SDL3 desktop shell is implemented; the game runtime remains a mock.

## Build and test

Requires C++20, CMake 3.25+, Ninja and Python 3.11+. From `editor/`:

```
cmake --preset dev
cmake --build --preset dev --parallel 2
ctest --preset dev
```

Use the `sanitizers` preset for ASan/UBSan, or the separate
`thread-sanitizer` preset for TSan. Do not enable both in one build directory. Configure with
`-DLYKO_CLANG_TIDY=ON` to run installed clang-tidy while compiling. On Windows use
a Visual Studio developer shell with Ninja and Python available. CMake, clang-tidy
and desktop checks run locally on Linux. Windows remains unverified. The tested
cloud helper prepares CMake/Ninja, Linux headers, clang-tidy and Xvfb without root:

```
bash editor/tools/setup-cloud.sh   # from repository root; network-enabled context
source /workspace/.lyko-tools/activate.sh
```

This helper is specific to the managed Debian 13 cloud environment. Network
operations may require approved execution outside the default network sandbox.


From the repository root, the verified fallback is:

```
mkdir -p /tmp/lyko-build
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Ieditor/src -Ithird_party/picosha2 editor/src/project_manifest.cpp editor/src/content_hash.cpp editor/src/asset_source.cpp editor/src/assets.cpp editor/src/prefab.cpp editor/src/command_bus.cpp editor/src/command_service.cpp editor/src/scene_io.cpp editor/src/mcp_server.cpp editor/src/text_file.cpp editor/src/ui/workspace_io.cpp editor/src/ui/command_queue.cpp editor/src/transport/mcp_stream.cpp editor/src/main.cpp -o /tmp/lyko-build/lyko
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Ieditor/src editor/src/command_bus.cpp editor/tests/command_bus_tests.cpp -o /tmp/lyko-build/tests
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Ieditor/src editor/tests/json_tests.cpp -o /tmp/lyko-build/json-tests
/tmp/lyko-build/tests
/tmp/lyko-build/json-tests
python3 editor/tests/cli_tests.py /tmp/lyko-build/lyko
python3 editor/tests/persistence_tests.py /tmp/lyko-build/lyko
python3 editor/tests/mcp_tests.py /tmp/lyko-build/lyko
python3 editor/tests/feedback_tests.py /tmp/lyko-build/lyko
python3 editor/tests/workspace_tests.py /tmp/lyko-build/lyko
python3 editor/tests/snapshot_tests.py /tmp/lyko-build/lyko
```

Use the existing isolated checkout; do not create a worktree unless requested.
The GitHub workflow is manual-only and requests self-hosted runners with Linux
or Windows and X64 labels. It assumes these prerequisites are installed and runs
no artifact uploads. Joshua must arrange/authorize runners before dispatching it.
No hosted CI runs or paid services have been requested.

## Headless JSON CLI

Start `lyko --instance demo`, then send one JSON object per line:

```json
{"instance":"demo","command":"commands.list"}
{"instance":"demo","command":"entity.create","arguments":{"name":"Player"},"expected_revision":0}
{"instance":"demo","command":"play.start"}
{"instance":"demo","command":"play.input","arguments":{"value":"right"}}
{"instance":"demo","command":"logs.query"}
{"instance":"demo","command":"preview.capture"}
{"instance":"demo","command":"scene.query"}
{"instance":"demo","command":"history.undo"}
```

Responses contain `ok`, `revision` and either `data` or an `error` with `code`,
`field` and `message`. The CLI continues after invalid requests and exits 1 if
any failed. Messages and scene files are capped at 1 MiB, JSON nesting at 64.
IDs and revisions require exact unsigned integers; numbers are never coerced.
Requests reject unknown fields. Discovery returns command versions and input
schemas. Read [the command protocol](../docs/COMMAND_PROTOCOL.md) for transactions,
persistence and MCP integration.

History is in memory, using full before/after snapshots. It is not persisted and
has not been optimized for large projects. The mock records at most 1024 input
messages per session and retains the latest 256 log messages. A scene load resets
runtime state. Mock screenshots are deterministic 64x36 PPM images representing
state, not rendered gameplay. GUI frame-rate and handheld usability are untested.

## Desktop shell model

The toolkit-independent workspace supports selection/inspector data, five panel
positions, split sizing, rebindable shortcuts and palette search through the same
command service. The desktop now consumes these models through asynchronous commands.

```json
{"instance":"demo","command":"workspace.configure","arguments":{"scale_percent":150,"theme":"high_contrast","reduced_motion":true}}
{"instance":"demo","command":"panel.dock","arguments":{"panel":"inspector","dock":"bottom"}}
{"instance":"demo","command":"selection.set","arguments":{"id":1}}
{"instance":"demo","command":"selection.query"}
{"instance":"demo","command":"palette.search","arguments":{"query":"undo"}}
{"instance":"demo","command":"workspace.save","arguments":{"path":"my-workspace.toml"}}
```

Selection requires an existing entity; ID zero clears it. Workspace saves require
a new path, just like scene exports. `workspace.load` restores TOML settings and
layout as one undoable transaction without replacing the scene. Scene loads
preserve preferences/layout and clear selection. See ADR 0003 for scope and limits.

## Desktop adapter execution

`ui/command_queue.hpp` provides a single-worker asynchronous adapter for a future
window. Keep CommandService and its backend alive until the queue is destroyed.
Submit requests with `try_submit`, correlate accepted tickets with `try_pop`, and
render cached responses. The default 64-request limit includes unpolled results.
A full/busy queue accepts nothing; do not retry accepted requests automatically.
Close cancels queued work, lets running work finish and joins on destruction.

`editor.snapshot` supplies a consistent revision of all panel models, including
history entries with transaction command names and undo/redo state. It excludes
software capture and file I/O. Fetch preview images separately using the same
expected_revision. These core facilities have no GUI dependency. The optional desktop consumes
them, but a measured frame-rate target has not yet been verified. See ADR 0004 for lifecycle rules.

The deterministic queue test can also be built directly from the repository root:

```
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Ieditor/src -Ithird_party/picosha2 editor/src/project_manifest.cpp editor/src/content_hash.cpp editor/src/asset_source.cpp editor/src/assets.cpp editor/src/prefab.cpp editor/src/command_bus.cpp editor/src/command_service.cpp editor/src/scene_io.cpp editor/src/mcp_server.cpp editor/src/text_file.cpp editor/src/ui/workspace_io.cpp editor/src/ui/command_queue.cpp editor/src/transport/mcp_stream.cpp editor/tests/command_queue_tests.cpp -o /tmp/lyko-build/queue-tests
/tmp/lyko-build/queue-tests
```

`CommandQueue::try_status` provides non-waiting queued/running/ready counts for busy indicators.

## Visible desktop and shared-instance MCP

From `editor/`, with platform development dependencies available:

```
cmake --preset desktop
cmake --build --preset desktop --parallel 4
ctest --preset desktop
build/desktop/lykoware_editor --instance my-editor
build/desktop/lykoware_editor --instance my-editor --mcp
```

On Windows, run the `.exe` from a Visual Studio developer shell. For the rootless
cloud sysroot use the setup helper, which supplies its CMake search paths. Launch
the window on an available DISPLAY. Tests use SDL dummy video without a display.
`--mcp` takes newline-delimited MCP on stdin and emits only protocol JSON on stdout;
its calls affect the same service/state rendered by the window. EOF closes the
transport, not the window. Different process instances still have separate state.

The first shell supports entity creation/selection/rename, play/stop, preview,
history, theme and command-driven panel placement. Ctrl+K browses all commands;
primitive arguments use generated fields, dropdowns and checkboxes with inline
validation. transaction.execute opens a typed batch builder: add registered
mutations, edit fields, reorder/remove rows, then execute once. A batch failure
leaves all rows unapplied; undo/redo acts on the whole batch. The GUI allows up
to 64 rows. Layout objects provide typed split/tab/panel controls plus advanced JSON import.
Dockspace resizing, splitting, merging, tab reordering and active tabs share
undo/redo with CLI/MCP layout.replace. Workspace exports write TOML v2 and read
v1/v2. Floating-window drops restore the confirmed layout with an inline error.
Legacy panel.dock/layout.resize commands reset custom trees to starter presets.
A first font/icon polish pass is implemented; hardware frame-rate and full
accessibility claims remain unverified.

Opt-in test harnesses (not production capture commands):

```
SDL_VIDEODRIVER=dummy build/desktop/lykoware_editor --smoke
SDL_VIDEODRIVER=dummy build/desktop/lykoware_editor --smoke-input
build/desktop/lykoware_editor --smoke-input --capture /tmp/new-desktop.bmp
```

The capture path must be new; X11 capture requires an active display. The input
harness injects SDL events for Create, Ctrl+Z, redo, Ctrl+K and inspector rename,
then verifies scene state. BMP output is a local test artifact; nothing uploads it.

## Generated command forms

The registry now publishes defaults, choices, titles, ranges and UTF-8 byte limits
alongside argument types. Palette controls use those schemas; settings and rename
forms prefill values from the current snapshot. Invalid fields disable execution.
Shortcuts bound to commands with parameters open a form instead of sending empty
arguments. Ctrl+K focuses search; Escape goes back or closes it.

The draft model is independent of ImGui and never edits scene/history until an
explicit command submission. Domain errors such as stale revisions, shortcut
conflicts and nonexistent entities still come from the authoritative service.
Batch commands use typed rows; layout objects use typed controls with JSON import.
A general recursive schema renderer remains future work.
The --smoke-input harness additionally opens workspace.configure, edits reduced
motion with SDL mouse events and checks that submission updates the workspace.

`--smoke-docking` exercises native resize, merge, tab reorder/selection, split,
undo/redo and unsupported floating-drop rollback through synthetic SDL input.

The --smoke-input scenario also opens the batch builder through palette search,
adds two entity.create rows and checks atomic submission, undo and redo. Drafts
are transient; Back or a new search discards them without a command.

Open **Shortcuts** from the top bar to browse all commands and current bindings,
including unbound actions. Search by command, description or shortcut. **Edit**
opens the existing keymap.bind form with current values; execute to apply or leave
Shortcut empty to unbind. Conflicting keys are rejected without changing state.
Undo/redo restores bindings. Tooltips and the inspector undo hint follow the
confirmed keymap. Parameterized shortcuts open argument forms. Entity right-click
menus offer Inspect, Rename, Undo and Redo with current shortcut labels.

The input smoke test additionally edits Undo to Ctrl+U through the shortcut
browser, verifies the old key stops dispatching, and checks binding undo/redo.

Open layout.replace to edit a private layout draft: change split direction or share
(5–95%, with 0.01% precision), active tabs, tab order, panel placement/visibility,
or create another split. Groups are labelled by their panels. Execute applies the
whole draft as one undoable command. Batch rows use the same controls; a newly
added layout row starts from the starter layout rather than a live snapshot.

Advanced JSON import remains available. Text edits must be imported or discarded
before Execute becomes available. Invalid imports preserve the valid draft and
leave live state untouched. Invalid typed edits also preserve the draft; correct
the error or choose Keep valid draft before executing. This is a specialized
layout widget, not a general recursive JSON Schema form renderer.

`--smoke-layout-form` checks typed percentage editing, explicit submission,
undo/redo and invalid JSON import/discard through native SDL events.

The desktop embeds pinned Apache-2.0 Roboto Regular (version 2.001047) and uses
original vector icons alongside text labels. The optional font resource wrapper
checks the vendored font checksum and generates an embedded header in the build
directory; no font files are fetched at build/runtime or loaded from the source
checkout. See third_party/README.md for provenance and the bundled licence.
Body text starts at 15px and follows UI scaling. Standard controls use the 32px
height token; this does not claim every small control meets a touch-target size.
Surface, hover/active, border and focus colours follow the design tokens in both
themes. Full Unicode glyph coverage and screen-reader support remain future work.

Palette forms now show waiting, completion and failure feedback beside the draft.
A matching service field error also appears under its control; expand Error details
for structured code/field/request information. Service errors preserve the draft;
edit it and explicitly submit again. Pending form edits/Execute are disabled to
prevent duplicate submissions. Back/search remains available, and leaving a form
does not cancel an already submitted command. Late replies do not replace another
form's feedback. Query results have an expandable 4 KiB preview with a truncation
marker; the underlying structured response is preserved.

`--smoke-feedback` exercises a rejected shortcut binding, correction/retry and a
second rejection through native SDL input, checking feedback and revision behavior.

Entity context menus now include Duplicate and Delete. Duplicate opens a source/name
form and selects the new copy on commit. Delete acts on the clicked entity and
clears selection if necessary. Both are undoable and available as entity.duplicate
and entity.delete through CLI/MCP, the palette, rebindable keys and batch rows.
The current entity model contains names only; no runtime resources are duplicated.

Use entity.reparent with id and parent (0 for root) to build a scene hierarchy.
The Scene panel indents entities in parent-first order and offers Reparent in its
context menu. scene.tree exposes parent IDs/depths to CLI/MCP; editor.snapshot
contains scene_tree. Cycles and depth above 64 are rejected. Parent deletion
promotes direct children to root; duplicate remains beside its source and copies
no descendants. Undo restores hierarchy and selection atomically. Scenes with
parent links save as JSON v2; v1 still loads and flat scenes still export as v1.
No transforms, runtime parenting or engine integration are implemented yet.

Select an entity and choose Edit local transform in Inspector to open typed
position, Euler-degree rotation and scale fields. Execute submits one atomic
transform.set; transform.query reads the same values through CLI/MCP. Signed
fractional numbers are supported, nonfinite values and zero scale are rejected.
Duplicate/Delete and undo/redo include local transforms. Reparent keeps local
values; transform.world computes world matrices, while mock-preview transformation
remains unimplemented.
Scenes with nonidentity transforms save as JSON v3; v1/v2 scenes still load.

Inspect world matrix in Inspector opens transform.world for the selected entity.
Execute shows its matrix/convention metadata in the existing Result data panel.
CLI/MCP expose the same query. Matrices use right-handed Y-up/-Z-forward editor
coordinates, row-major storage and column vectors; local order is T*Rx*Ry*Rz*S.
Shear/reflection are retained. This is a computed query, not a world-space editor,
reparent-with-world-preservation operation or real-renderer integration.

Use entity.reparent_world (or Reparent (preserve world) in an entity context menu)
to keep subtree world placement while changing parent. It applies one undoable
hierarchy/local-transform transaction. Shear or unsafe numerical cases are rejected
with a clear error and the scene stays unchanged. Preservation uses a documented
component-wise 1e-9 relative/absolute tolerance; it is not exact arithmetic.
The original entity.reparent still keeps local values and is always available.

Export prefab in an entity context menu opens prefab.export with the clicked ID.
Choose a new JSON destination; the complete authoring subtree is captured with
root-local placement. Use prefab.instantiate in the palette, CLI, MCP or a batch
to copy it under parent (0 for root). Fresh IDs and internal links are assigned
atomically, the copied root is selected, and one undo removes the entire instance.
These first prefabs are detached authoring copies, with no live links, overrides
or runtime resource cloning. Existing export files are never overwritten.

Open Assets in the top bar for the modeless catalogue browser. Register adds a
project-relative path and declared kind; Relocate changes metadata while keeping
its ID; Forget removes metadata undoably. All open the same command forms exposed
by CLI/MCP and batch discovery. Search matches paths/kinds, and table rendering
clips offscreen rows. Load/Save use a separate JSON catalogue; saving needs a new
path and loading replaces only catalogue state. Scene loading preserves it.

This is metadata infrastructure, not file import or a filesystem browser. Paths
need not exist, declared kinds are not decoded, and no sources are copied, moved
or deleted. IDs are stable within the saved catalogue rather than global GUIDs.
`--smoke-assets` opens the browser, forgets a row through its typed form, then
undoes and verifies the row returns using native SDL input.

Validate in the Assets browser opens asset.validate with the asset ID prefilled.
Enter an absolute project directory; the command reads the registered scene or
prefab file, checks observed canonical containment and its authoring format, then
shows byte/entity counts through command feedback. Unsupported kinds are disabled
in the browser and return unsupported_asset_kind through CLI/MCP. Missing, malformed,
nonregular and oversized sources return structured errors without changing state.
This is read-only validation, not import or a guarantee against concurrent hostile
filesystem changes. The project root is explicit per call and is not persisted.

Instantiate in a prefab catalogue row opens asset.instantiate with the catalogue
ID prefilled. Supply the explicit project directory and destination parent (0 for
root). The validated source becomes a detached undoable authoring copy, exactly as
with prefab.instantiate; source files and catalogue entries are unchanged.
Registration/relocation must commit before instantiating by catalogue ID. A batch
that changes that entry first is rejected instead of using stale path metadata.
Redo works after the source file disappears because committed authoring data is
captured. No linked definition, resource cache or engine object is created.

Successful source validation now shows a selectable SHA-256 fingerprint. Recheck
this version opens asset.check with ID/root/digest prefilled. For prefabs,
Instantiate validated bytes opens asset.instantiate_checked with the same token;
if source bytes have changed, the command refuses the mutation and asks for a new
validation/review; Validate current source keeps the ID/root prefilled for that
review. Exact file bytes are hashed, including whitespace. Checked
instantiation remains a detached copy; redo never rereads the source.
PicoSHA2 v1.0.1 is vendored under MIT behind the content_hash wrapper; see
third_party/README.md. Fingerprints are observations, not signatures or file locks.


### Audit registered sources

Choose **Audit sources...** in the Assets browser, or send:

```json
{"instance":"studio","command":"asset.audit","arguments":{"root":"/absolute/project"}}
```

The same command is available through MCP. Reports list catalogue entries by ID,
with valid/invalid/skipped counts and per-entry diagnostics or exact-byte hashes.
Unsupported kinds are skipped without reading them. A completed report can contain
invalid entries; inspect the counts. Desktop feedback shows the summary and a
bounded detail preview; CLI/MCP return all rows. No files, catalogue state or undo
history change. Large audits run on the worker and occupy its command queue until
finished. Sources are observed sequentially rather than as one filesystem snapshot.


The desktop audit form now includes the full report as a searchable table. Filter
by status, then select a row to inspect diagnostics or its exact-byte fingerprint.
**Validate current source...** opens the shared validation form; a valid prefab
also offers **Instantiate audited bytes...**, which preloads checked instantiation
with the report's digest. Commands still validate current data when submitted.
Changing the root clears obsolete feedback. Report navigation opens another form;
reports are transient observations and are not saved or automatically refreshed.


### Project manifest foundation

Search the palette for `project.manifest.write` or `project.manifest.validate`.
The same commands work headlessly and through MCP:

```json
{"instance":"studio","command":"project.manifest.write","arguments":{"path":"/project/lykoware.toml","project_id":"0123456789abcdef0123456789abcdef","name":"My Game","directories":{}}}
{"instance":"studio","command":"project.manifest.validate","arguments":{"path":"/project/lykoware.toml"}}
```

The parent directory must already exist and the destination must be new. Use your
own stable 32-character lowercase hexadecimal identity; the example is illustrative.
Directory settings default to assets, scenes, scripts, graphs and .lyko/cache.
Override roles in the directories object; paths are portable and project-relative,
and generated cache directories cannot overlap source directories. Configuration
does not create those folders, import files or activate a project. Active project
creation/open/close is the next lifecycle slice.

The conventional manifest is `lykoware.toml`, schema version 1. Supported syntax is
single-line basic/literal strings, bare known keys, comments, LF/CRLF and one optional
[directories] table. Unicode scalar escapes are supported; unknown/duplicate keys,
unsupported versions and other TOML constructs fail explicitly. Generated files
are compatible with Python tomllib and deterministic; no new library is required.


### Reusable project fixtures

[Fixture data and expectations](tests/fixtures/README.md) cover all currently
supported persisted formats: scene v1/v2/v3, workspace v1/v2, prefab v1, catalogue
v1 and project manifest v1. The `fixture_projects` CTest group exercises CLI/MCP
parity, invalid-candidate rollback, scene promotion and workspace migration.
An offline generator provides a 1000-entity scene and a full 4096-entry catalogue
in a fresh temporary directory. These are correctness workloads, not hardware
performance results. No engine files or runtime resource import are involved.
