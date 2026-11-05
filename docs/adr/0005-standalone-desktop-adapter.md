# ADR 0005: Standalone software desktop adapter

Status: Implemented locally; review pending

## Decision

Build the optional `lykoware_editor` executable with Dear ImGui v1.92.5-docking
(MIT) and SDL3 release-3.2.28 (zlib), pinned by upstream commit. Vendor the ImGui
core/backend subset unchanged and SDL's tracked source as a checksum-verified
archive. The wrapper performs offline extraction into the build directory.
Headless builds never configure or link either library. See third_party/README.md
for source provenance and bundled-component licence records.

Use SDL's software renderer for the shell and deterministic mock preview. Disable
Vulkan, OpenGL, camera, audio and device subsystems not needed by this milestone.
The desktop platform wrapper owns SDL/ImGui lifetime, event translation, preview
texture upload and test capture. This introduces no external engine dependency.

Render scene, inspector, viewport, console and history from editor.snapshot.
Buttons, selection, committed inspector edits, themes and panel-location menus
submit named commands through CommandQueue. Forms, hover/focus and palette search
text are transient input drafts, not independently persisted document mutations.
The palette discovers every command and its shortcut/description; the initial
parameter editor was JSON; ADR 0006 supersedes this with schema-driven primitive
widgets and an advanced JSON editor only for arrays.

Use the starter slot/split workspace model for docked panels. Native undocking,
arbitrary split dragging and resizing are disabled until these changes can be
represented and validated as undoable commands. Panel menus and layout.resize
already change the saved model through the bus. This is not complete free-form
docking. ImGui ini persistence is disabled to avoid an alternate state path.

Opt-in `--mcp` runs a cancellable stdio reader on a separate thread against the
same CommandService as the visible desktop. The UI refreshes snapshots periodically
and handles revision conflicts without replaying accepted edits. An idle client
cannot hold shutdown open indefinitely; an executing command may still finish.
EOF/disconnected output closes the protocol transport without killing the window.
There are no default network listeners. On POSIX, MCP mode ignores SIGPIPE so an
agent disconnect does not terminate the desktop.

## Validation

CMake headless and desktop builds pass locally on Linux. Desktop tests render with
SDL dummy video, check shared-instance MCP calls and disconnect handling, and
inject SDL mouse/key/text events for create, undo, redo, Ctrl+K and inspector rename.
A separate Xvfb run verifies a real X11 window and captures its frame for inspection.
The cloud script prepares rootless tool/system caches and can be rerun safely.

## Remaining work

Windows compilation/runtime, target-device testing, measured 60fps performance,
screen-reader support, consistent icon assets, richer fonts, generated parameter
forms, free docking and final visual polish are not validated by these checks.
The mock preview is simulated state, not a game renderer. No hosted CI or paid
services have been requested; manual CI still needs approved self-hosted runners.
