# Editor roadmap

1. Review ADR 0001 and establish standalone C++20/CMake presets and test harness.
2. Implement typed command registry, structured errors, atomic transactions,
   undo/redo, EngineBridge and deterministic mock. Add headless CLI and tests.
3. Vendor reviewed, pinned permissive GUI dependencies and wrappers. Build the
   window, docking shell, design tokens, palette, keymaps and accessibility settings.
4. Add instance-targeted MCP tools and versioned JSON scene save/load through the
   same registry. Test transport behavior, persistence and frontend parity.
5. Add mock play/input/log/scene/screenshot feedback and a software preview.
6. Validate UI performance and usability on desktop and handheld hardware.

Keep changes small and reviewable. Joshua merges; no engine integration is
scheduled or authorized. CI must cover Windows, Linux, sanitizers and clang-tidy
without mandatory artifact uploads or paid services.

## Current checkpoint

The command core, JSON CLI, scene/workspace persistence, shared-instance desktop
MCP, async execution, software preview and first ImGui/SDL3 desktop are implemented
and tested locally on Linux. CMake and clang-tidy checks now run successfully.
Desktop smoke tests include actual SDL input and a separately inspected X11 frame.

Schema-driven primitive palette forms, contextual defaults and parameterized
shortcuts are now implemented and covered by model and SDL interaction tests.

Transaction-backed split/tab docking, active tabs, undo/redo and TOML v2 layout
persistence are implemented; v1 settings migrate on load. Floating windows remain
a future extension.

Typed transaction rows with add/remove/reorder and atomic submission are now
implemented and covered by model and native SDL interaction tests.

A searchable shortcut browser, binding edit links, live action hints and contextual
Rename now improve command discoverability. Bindings still use the shared bus.

Layout object forms now expose split/tab/panel controls with explicit JSON import
and draft validation. A general recursive schema renderer remains future work.

The first visual polish pass adds embedded Apache-2.0 Roboto, a consistent original
vector icon set, token-sized controls and matching interactive surface colours.

Form-local command feedback now correlates queue tickets/drafts, displays field
errors and query previews, and supports explicit correction/retry.

Basic entity Duplicate/Delete actions now share atomic selection/history behavior
across desktop, CLI and MCP.

Authoring hierarchy now supports validated reparenting and deterministic scene-tree
queries with v1/v2 text persistence. Spatial transforms remain future work.

Local authoring transform commands, numeric forms and JSON v3 persistence now
provide position/rotation/scale data. World composition remains future work.

Next: broader object schemas, further usability polish, Windows and target
hardware validation. Prefab/asset schemas, procedural graphs, glTF import and binary
cooking remain later milestones. CI is manual/self-hosted and has not been run.
