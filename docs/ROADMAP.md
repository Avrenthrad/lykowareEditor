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

Next: generated friendly palette forms, context menus and shortcut discoverability,
transaction-backed flexible docking, icons/fonts/visual polish, Windows and target
hardware validation. Prefab/asset schemas, procedural graphs, glTF import and binary
cooking remain later milestones. CI is manual/self-hosted and has not been run.
