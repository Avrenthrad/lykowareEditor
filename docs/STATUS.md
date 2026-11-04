# Editor status

## Completed

- Inspected the supplied checkout: its only source file was a title-only README.
- Added proposed ADR 0001, staged roadmap and future engine requirements.
- Established strict standalone scope; no engine integration or dependency.

## Foundation slice

- Added `editor/` C++20 core, CMake dev/sanitizer presets and a headless `lyko` CLI.
- Typed mutations share a command bus with atomic batch validation, structured
  errors, explicit instance targeting, optimistic revision checks and undo/redo.
- Added pure-value EngineBridge and mock play/stop/input backend.
- GCC strict-warning builds passed. Transaction/history/backend tests and the
  CLI feedback loop plus five invalid-input cases passed.
- ASan/UBSan tests passed with leak detection disabled: LeakSanitizer cannot run
  under the environment's process tracing. Leak checking remains unverified.
- No external libraries or engine dependencies were introduced.

## JSON command contract slice

- Replaced the prototype text CLI with bounded JSON-lines requests/results.
- Centralized command names, schema discovery and typed decoding in CommandService;
  the service serializes target/revision validation, execution and queries.
- Added atomic transaction requests, strict unknown-field rejection and Unicode
  handling. Entity IDs/revisions require exact unsigned 64-bit integers; signed
  and fractional JSON numbers parse without being coerced into IDs.
- Strict GCC builds and the core suite passed. The CLI suite passed discovery,
  Unicode, feedback/undo/redo and 15 malformed/invalid-request rollback cases.

## Scene persistence slice

- Added versioned authoring JSON with one entity per line, stable IDs and next-ID
  metadata; runtime state is deliberately excluded.
- `scene.load` validates the entire document before an undoable replacement.
- `scene.save` publishes a complete file atomically to a new path using a hard
  link. Existing files are preserved. Filesystems without hard links fail safely;
  overwrite support and power-loss durability are not implemented.
- Filesystem tests passed round trips, load undo/redo, existing-file preservation,
  malformed-scene rollback and staging cleanup. Input/output files are capped at
  1 MiB for this initial implementation.

## MCP transport slice

- Added opt-in stdio MCP mode: `lyko --instance NAME --mcp`.
- Implements initialization, tool discovery from command schemas, tool calls,
  structured command results and explicit instance targeting. No network listener.
- Black-box tests passed lifecycle, discovery, signed request IDs, target/revision
  rejection, notification safety and equivalent CLI/MCP results.
- Advertises protocol versions 2024-11-05 and 2025-06-18; external MCP-client
  compatibility testing remains outstanding.

## Mock feedback and build-check slice

- Added bounded mock logs and deterministic 64x36 PPM software capture commands.
  Tests verify play/input/log/query/capture plus visual and log undo/redo.
- Added JSON number/Unicode/structure contract tests and a clang-tidy build option.
- Added manual-only self-hosted Linux/Windows CI configuration with sanitizer and
  clang-tidy checks. No workflow was dispatched and no artifact uploads exist.
- Updated run instructions and documented the actual JSON/MCP/persistence protocol.
- Added ADR 0002 covering protocol bounds, scene publication and manual CI.
- Strict GCC, JSON contract, core, CLI, persistence, MCP and feedback checks pass.
  Core tests include concurrent stale writes and bounded mock history/logs.

## Current limitations

- Git access to this repository fails because the environment proxy on port 8080
  is unreachable. Latest remote main cannot be verified and no PR can be opened.
- Local starting commit: `8308edf` (Initial commit), branch `work`.
- GCC 14.2 and Make are present; CMake and Ninja are absent. Installation and
  dependency acquisition remain blocked by network access.
- CMake presets, Windows builds and clang-tidy remain unrun because the required
  tools/platform are unavailable. CI is configured but unexecuted; approved
  self-hosted runners are required. No dependencies have been vendored.
- GUI/docking, prefab/asset schemas, TOML config and glTF import are outstanding.
  CLI JSON requests and registry input schemas are implemented; transport and
  persistence contracts will be extended in subsequent slices.
- ADR remains proposed for review; it describes the target architecture.

## Next handover

Restore environment proxy connectivity, fetch this repository's main, reconcile
local commits with remote changes, then validate CMake presets and add the
desktop shell using reviewed, pinned permissive dependencies. Validate external
MCP client compatibility and complete prefab/asset/config schemas. Use the
existing isolated checkout; do not create a worktree unless explicitly requested.
Do not access the engine repository. Record progress here after each task.
