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

## Current limitations

- Git access to this repository fails because the environment proxy on port 8080
  is unreachable. Latest remote main cannot be verified and no PR can be opened.
- Local starting commit: `8308edf` (Initial commit), branch `work`.
- GCC 14.2 and Make are present; CMake and Ninja are absent. Installation and
  dependency acquisition remain blocked by network access.
- CMake presets, Windows builds and clang-tidy remain unrun because the required
  tools/platform are unavailable. CI has not been configured or executed; paid
  runners/services have not been invoked. No dependencies have been vendored.
- GUI/docking, MCP, logs and screenshots are outstanding.
  CLI JSON requests and registry input schemas are implemented; transport and
  persistence contracts will be extended in subsequent slices.
- ADR remains proposed for review; it describes the target architecture.

## Next handover

Restore environment proxy connectivity, fetch this repository's main, reconcile
these documents with remote changes, then validate CMake presets and add the remaining shell and transport work. Use the
existing isolated checkout; do not create a worktree unless explicitly requested.
Do not access the engine repository. Record progress here after each task.
