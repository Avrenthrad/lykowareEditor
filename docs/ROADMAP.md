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

ADR 0001 is drafted; ADR 0002 records headless protocol/publication choices.
The command core, JSON CLI, scene JSON persistence, stdio MCP and deterministic
mock feedback loop are implemented and tested locally. Desktop shell, prefab and
asset schemas, TOML configuration and glTF import remain outstanding. CMake,
Windows and clang-tidy validation require missing tooling/platform support;
manual self-hosted CI is configured but has not run. Restore the cloud proxy
before acquiring reviewed GUI dependencies or reconciling with remote main.
