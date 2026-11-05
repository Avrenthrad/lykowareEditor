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
- This checkpoint implemented the shell model; the actual window follows below.

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

## Desktop shell and environment milestone

- Added a real optional ImGui/SDL3 window, docked scene/inspector/viewport/console/
  history panels, Ctrl+K palette, inspector rename, undo/redo, workspace menus and
  mock preview texture. All committed editor changes use the command service.
- Vendored pinned MIT ImGui and zlib SDL sources with provenance, licence records
  and an offline checksum-verified CMake wrapper. Headless builds stay independent.
- Added desktop --mcp against the same visible instance, safe EOF/disconnect
  handling and periodic snapshot refresh. No network listener is opened.
- CMake dev builds and all nine headless tests pass. All 12 desktop-preset checks
  pass, including render,
  shared-MCP/disconnect and synthetic SDL interaction checks pass. A real X11/Xvfb
  window passed the same mouse/key/rename interaction sequence and was captured
  for visual inspection. The interaction harness creates an
  entity, undoes/redoes, opens Ctrl+K and renames through the inspector.
- The actual sanitizer preset builds with clang-tidy enabled and passes all nine
  tests with leak detection disabled for the environment tracing restriction.
  Queue tests also pass under ThreadSanitizer. Clang-tidy reports no project-code
  findings. Windows and remote CI have not run.
- Added tested, repeatable rootless cloud setup under editor/tools/setup-cloud.sh.
  The environment install_script and start_skill are saved as a review draft;
  publication and restoration in a fresh task have not been verified.
  Tools/system packages live outside the checkout in /workspace/.lyko-tools and
  /workspace/.lyko-system. Network works with approved unsandboxed execution; prior
  proxy-failure diagnoses were incorrect. No environment restart was needed.

## Schema-driven palette forms

- Added registry-owned defaults, titles, enum choices and numeric constraints,
  published through both CLI discovery and MCP. UTF-8 byte limits are explicit.
- Added a toolkit-independent draft form model plus desktop text, uint64, checkbox
  and dropdown controls. Inline errors prevent invalid draft submission; the bus
  remains authoritative for domain validation and atomic changes. Direct named
  C++ service calls also reject malformed UTF-8 before writing.
- Workspace/rename forms prefill current values. Parameterized shortcuts open
  forms; Ctrl+K focuses search. Array transactions retain an advanced JSON editor.
- All 13 desktop-preset tests pass. The SDL interaction harness searches, opens,
  edits and submits workspace.configure through the generated controls.
- All 10 headless/model tests pass under ASan/UBSan with the documented local leak
  limitation. Clang-tidy reports no project-code findings. ADR 0006 records scope.

## Review and environment handover

- Prepared the private codex/standalone-editor-* review branches, including forms; see
  docs/REVIEW_STACK.md for the intended PR bases. Main is untouched.
- Draft PR creation is blocked by api.github.com network policy (CONNECT 403).
  Added this domain to the environment configuration draft, preserving existing
  rules/presets. It is not applied or published; API authentication is untested.
- install_script/start_skill are saved in the same draft. Current-instance setup
  and repeatability are verified; fresh-task restoration/publication are not.

## Transaction-backed docking

- Added validated split/tab trees and layout.query/replace/reset through the shared
  command service, CLI and MCP. Native resize, merge, tab reorder/selection and
  split gestures commit one undoable transaction with revision conflict checks.
- Added workspace TOML v2 persistence with v1 migration. Canonical trees prune
  empty regions, enforce panel coverage and keep stable structural path IDs.
- Desktop restores confirmed state after rejected or unsupported floating drops.
  Window resizing preserves relative splits without recording a layout edit.
- All 15 desktop-preset tests and 11 ASan/UBSan headless tests pass (local leak
  detection disabled as above). Clang-tidy reports no project-code findings.
  Native SDL interactions and an X11/Xvfb capture were checked. The harness covers
  resize, merge, reorder, active tabs, splitting, undo/redo and floating rollback.
- Extracted opt-in interaction scenarios from main; frame pacing now waits only
  for the remaining frame budget. Hardware performance remains unmeasured.
- ADR 0007 documents the model and migration. No engine integration occurred.

## Typed transaction builder

- Added a palette builder for transaction.execute with registry-derived mutation
  choices, typed child fields, row reordering/removal and one atomic submission.
  Queries, undo/redo, exports and nested transactions are excluded from choices.
- Added toolkit-independent batch draft validation with numbered row errors and
  a 64-row GUI limit. The command service remains authoritative for scene/domain
  constraints; no draft changes state or history.
- Model checks cover exclusions, missing/invalid fields, row bounds/order,
  single-step undo/redo and full rollback after a later row fails.
- Extended SDL interaction coverage: search/open builder, add two mutation rows,
  submit, undo both rows together, redo both, then undo again.
- All 15 desktop checks pass; form tests also pass under ASan/UBSan with the
  documented local leak-check limitation. The batch interaction sequence passes
  in X11/Xvfb. Clang-tidy emits no project-code findings.
- ADR 0008 records the scope. Layout objects retain advanced JSON controls;
  recursive transactions remain disallowed by the shared service.

## Shortcut discoverability

- Added a modeless, searchable Shortcuts browser over every registered command,
  including unbound actions. Search matches command names, registry descriptions
  and current bindings, with ASCII case folding and deterministic name ordering.
- Edit opens the existing keymap.bind form with command/current shortcut prefilled.
  Drafts do not bind keys; explicit execution uses the same revision-guarded queue
  and service. Empty shortcuts unbind; existing conflict checks remain authoritative.
- Buttons and entity context actions now show registry documentation and current
  bindings in tooltips. The inspector undo hint and palette rows read the confirmed
  keymap instead of hard-coded or cached binding labels. Added contextual Rename.
- History buttons reflect confirmed undo/redo availability. No service commands,
  persistence format, dependency or engine integration changed.
- Model tests cover registry documentation, all/unbound commands, case-insensitive
  binding search, rebinding/unbinding and conflict rejection. SDL input searches
  the browser, edits Undo to Ctrl+U, confirms Ctrl+Z stops dispatching, then checks
  undo/redo restores each binding. ADR 0009 records the presentation contract.
- All 15 desktop-preset tests pass. The model/form tests pass under ASan/UBSan
  with the documented local leak-check limitation; clang-tidy reports no project
  findings. The same browser/rebinding sequence passes in X11/Xvfb and its
  resulting frame was inspected.

## Typed layout object forms

- Added a layout-specific nested object editor for layout.replace, including batch
  rows: split direction/share, active tabs, tab order, panel grouping/hiding and
  creation of new splits. Standalone forms start from the confirmed layout;
  newly added batch rows use a valid starter draft.
- Added a toolkit-independent LayoutForm draft model that validates candidate
  edits through the existing layout validator before replacing its private tree.
  No edit touches live state/history until one explicit command submission.
- Retained advanced JSON import with an explicit import/discard boundary. Dirty
  or invalid JSON blocks execution and typed editing until resolved; invalid
  typed edits preserve the valid tree and require correction or acknowledgement.
- Model tests cover structural edits, exact ratios, active tabs, rejected edits,
  canonicalization, all-hidden/restore, JSON roundtrip and one-step bus undo/redo.
  Native input tests edit a percentage, verify no live change before Execute,
  check exactly one revision, undo/redo and malformed JSON import/discard gating.
- All 17 desktop-preset tests and all 12 ASan/UBSan groups pass (with the
  documented local leak-check limitation). Clang-tidy reports no project-code
  findings. The typed form/import scenario passes in X11/Xvfb; its final frame
  was inspected. Windows and target hardware remain unverified.
- ADR 0010 records the specialized widget and its limits. No generic recursive
  JSON Schema renderer, new protocol command, dependency or engine integration
  was introduced.

## Typography, icons and surface polish

- Vendored unmodified Roboto Regular (font version 2.001047, upstream commit
  38062f4b4a0be4346d07a928408da21602545e9e) under Apache-2.0. The font's own licence
  metadata and upstream licence were checked; provenance and SHA-256 are recorded
  in third_party/README.md. No newer differently licensed font was substituted.
- Added an optional desktop-only checksum-verified CMake resource wrapper. Font
  bytes are embedded, so the executable needs no adjacent font or source path.
  Headless builds remain independent of GUI resources.
- Applied a 15px Roboto body baseline, token-driven 32px normal controls, consistent
  spacing and explicit surface/hover/active/border colours. Added matching panel
  headings and ten original vector icons on one grid, with text labels and native
  button keyboard behavior retained.
- Expanded existing contrast checks across hover and active surfaces. Dark mode
  meets the existing 4.5:1 text target and high contrast meets 7:1 for normal/muted
  text on the checked surfaces. This is not a full accessibility certification.
- Inspected X11 captures at 100% dark and 150% high contrast. The scaled render
  was driven through the same-instance MCP command service from a different cwd.
- All 17 desktop tests and all 12 ASan/UBSan groups pass with the documented
  local leak-check limitation. Clang-tidy reports no project-code findings.
- ADR 0011 records the resource, design-token and icon decisions. No command,
  persistence, engine or backend contract changed. Full glyph coverage, Windows,
  hardware frame rate and screen-reader behavior remain unverified.

## Inline command feedback

- Added form-local waiting/completion/error feedback for normal and transaction
  forms. Matching service field errors appear beside controls; structured code,
  field and request details remain available without blocking dialogs.
- Correlate results by both draft generation and queue ticket. Late replies from
  an abandoned form cannot overwrite a new form, even for the same command.
  Navigation does not cancel commands already submitted.
- Disable form edits/Execute while that form is pending; retain Back/search
  navigation. Queue admission failures are shown locally and permit explicit
  retry. No mutation is automatically retried.
- Preserve drafts on service errors and clear obsolete feedback when arguments
  change. Successful queries expose a UTF-8-safe, bounded 4 KiB result preview;
  truncation does not alter the original structured response.
- Model tests cover generations/tickets, duplicate submission, local rejection,
  draft invalidation, retries and Unicode preview bounds. Native SDL input triggers
  a shortcut conflict, verifies the draft/revision are preserved, corrects the
  draft and submits successfully, then confirms a second conflict stays visible.
- All 19 desktop checks and all 13 ASan/UBSan groups pass with the documented
  local leak-check limitation. Clang-tidy reports no project-code findings.
  The conflict/correction scenario passes in X11/Xvfb and was visually inspected.
- ADR 0012 records the lifecycle and presentation rules. Queue metadata remains
  local to the adapter; CLI/MCP/service contracts and persistence are unchanged.

## Current limitations

- Windows compilation/runtime, hardware performance, screen-reader behavior and
  handheld input remain unverified. CI is manual-only and requires approved
  self-hosted runners; no paid jobs or artifact uploads have been invoked.
- This is a starter shell. Dockspace split/tab layouts are supported; floating
  windows are rejected and restored. Legacy panel.dock/layout.resize commands
  explicitly reset custom layouts to starter presets.
  Primitive palette parameters and batch rows use generated controls. Layout
  objects have typed controls plus JSON import; generic nested schema widgets
  remain future work.
- This is the first typography/icon pass. Further polish, prefab/asset schemas,
  glTF import and binary
  cooking remain outstanding. The software preview is explicitly simulated.
- Scene/workspace exports preserve existing files; in-place replacement and
  power-loss durability remain future work. JSON/TOML limits remain documented.
- ADRs are proposed/implemented locally for Joshua's review, not merged decisions.

## Next handover

Use only this repository and the existing isolated checkout; no worktree unless
explicitly requested and no external engine access. Activate cloud tools with
`source /workspace/.lyko-tools/activate.sh`; rerun editor/tools/setup-cloud.sh if
needed. See editor/README.md for desktop/MCP/headless commands. Continue with
broader object schemas, further usability polish and
Windows verification. Keep docs/STATUS.md current. Joshua merges.
