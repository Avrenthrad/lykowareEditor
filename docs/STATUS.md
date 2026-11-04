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

## Toolkit-independent workspace slice

- Added undoable selection, starter panel docking slots/splits, UI scaling,
  dark/high-contrast themes, reduced motion and conflict-checked key bindings.
- Added palette search across every registered command with current shortcuts and
  schemas, inspector selection queries and centralized toolkit-independent tokens.
- Added TOML workspace save/load with validated atomic replacement of in-memory
  settings and no-overwrite file exports. Scene and config loads preserve their
  respective boundaries; selection is excluded from config files.
- Shared bounded UTF-8 file handling between scene and workspace persistence.
- Strict GCC builds and workspace tests pass, including independent Python TOML
  parsing, contrast thresholds, transaction/undo/redo behavior, bad-config rollback,
  Unicode paths and CLI/MCP parity. ADR 0003 records scope and parser limitations.
- All seven test groups passed under ASan/UBSan with leak detection disabled
  because LeakSanitizer is incompatible with this environment tracing.
- The shell model is implemented; no actual GUI window or docking adapter exists.

## Desktop execution and panel snapshot slice

- Added CommandQueue worker execution over the existing service, FIFO tickets,
  non-waiting submission/polling, count backpressure including unpolled replies,
  and explicit shutdown cancellation of work not started.
- Added editor.snapshot with coherent scene/workspace/selection/token/log/history
  data, separate from preview capture and file I/O. Added history.query with named
  transaction records, original revisions and applied/redo status.
- Removed full-document copies from the service's initial revision check.
- Strict GCC builds and deterministic tests pass for blocked-backend submission,
  FIFO/bounds/errors/shutdown and snapshot/history/undo/redo/branch behavior.
- ADR 0004 and the command protocol document adapter lifetime and retry rules.
- Queue tests pass under ThreadSanitizer; queue/snapshot/workspace tests pass
  under ASan/UBSan with the previously documented leak-check limitation.
- Added a separate TSan preset and manual self-hosted CI step; CMake execution
  remains pending until newly installed tools are activated.
- Read-only Git access succeeds outside the default network sandbox. Earlier
  proxy failures were a sandbox restriction, not a broken environment proxy.
  Remote main remains at the original starting commit; GUI dependencies are now
  being acquired through the approved network path.

## Current limitations

- Default sandbox network requests fail; approved network-enabled commands work.
  Remote main has been checked and has no new commits beyond the starting point.
- Local starting commit: `8308edf` (Initial commit), branch `work`.
- GCC 14.2 and Make are present; CMake and Ninja are absent. Installation and
  dependency acquisition remain blocked by network access.
- CMake presets, Windows builds and clang-tidy remain unrun because the required
  tools/platform are unavailable. CI is configured but unexecuted; approved
  self-hosted runners are required. No dependencies have been vendored.
- GUI window/docking adapter, prefab/asset schemas and glTF import are outstanding.
  CLI JSON requests and registry input schemas are implemented; transport and
  persistence contracts will be extended in subsequent slices.
- ADR remains proposed for review; it describes the target architecture.

## Next handover

Restore environment proxy connectivity, fetch this repository's main, reconcile
local commits with remote changes, then validate CMake presets and add the
desktop shell using reviewed, pinned permissive dependencies. Validate external
MCP client compatibility and complete prefab/asset schemas. Python 3.11+ is
required for the independent TOML test parser. Use the
existing isolated checkout; do not create a worktree unless explicitly requested.
Do not access the engine repository. Record progress here after each task.
