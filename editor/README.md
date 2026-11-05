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
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Ieditor/src editor/src/command_bus.cpp editor/src/command_service.cpp editor/src/scene_io.cpp editor/src/mcp_server.cpp editor/src/text_file.cpp editor/src/ui/workspace_io.cpp editor/src/ui/command_queue.cpp editor/src/transport/mcp_stream.cpp editor/src/main.cpp -o /tmp/lyko-build/lyko
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
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Ieditor/src editor/src/command_bus.cpp editor/src/command_service.cpp editor/src/scene_io.cpp editor/src/mcp_server.cpp editor/src/text_file.cpp editor/src/ui/workspace_io.cpp editor/src/ui/command_queue.cpp editor/src/transport/mcp_stream.cpp editor/tests/command_queue_tests.cpp -o /tmp/lyko-build/queue-tests
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
to 64 rows. Layout objects retain an advanced JSON editor.
Dockspace resizing, splitting, merging, tab reordering and active tabs share
undo/redo with CLI/MCP layout.replace. Workspace exports write TOML v2 and read
v1/v2. Floating-window drops restore the confirmed layout with an inline error.
Legacy panel.dock/layout.resize commands reset custom trees to starter presets.
No font/icon
polish, hardware frame-rate or full accessibility claim is made yet.

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
Batch arrays use an advanced JSON field; richer nested editors remain future work.
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
