# Editor roadmap

**Maintained delivery checklist:** [All remaining steps and progress](DELIVERY_CHECKLIST.md).
Use its stable step IDs for task tracking; this file summarizes milestones and history.

## Delivery milestones and acceptance criteria

The [93-step checklist](DELIVERY_CHECKLIST.md) owns stable task IDs and completion
counts. The [work board](WORK_BOARD.md) owns the active queue, dependencies and
handover. STATUS.md records implementation evidence. A milestone is complete only
when its full checklist range and the observable acceptance criteria below pass.
Documentation of a milestone is not completion of that milestone.

| Milestone | Checklist steps | Acceptance criteria |
| --- | --- | --- |
| A. Consolidated foundation | 1–7 | Joshua's review/merge decisions are recorded; representative valid/invalid/migration projects exist; Windows and Linux builds and approved CI runs pass; real MCP clients and instance isolation are exercised; startup, frame, memory and command budgets have reproducible measurement procedures and numerical targets. |
| B. Safe project workflows | 8–16 | A project can be created, reopened and closed through shared commands; a versioned TOML manifest defines identity and directories; sources resolve against its root; dirty documents are visible; safe saves detect external changes; recovery and migrations preserve recoverable data; multiple documents have explicit history ownership; cancelling a long operation leaves committed state consistent. |
| C. Complete object authoring | 17–26 | Typed components, nested properties and references can be edited through UI/CLI/MCP; multi-selection, copy/paste, grouping, visibility and locking work atomically; duplication/persistence/prefabs preserve or intentionally remap references; invalid scenes produce actionable diagnostics; representative large scenes meet agreed budgets or have documented blockers. |
| D. Usable asset pipeline | 27–37 | Portable identities survive supported moves; import settings and derived data are versioned; glTF and supported textures/materials import with diagnostics and previews; dependencies, missing references and source changes are observable; reimport/cancellation are safe; linked prefab updates and override apply/revert support validated atomic history. |
| E. Scripting and procedural authoring | 38–49 | The approved permissive scripting system exposes the shared command registry; scripts and graph text can be created, validated and debugged headlessly; visual edits operate on the same versioned graph data; type/cycle errors are actionable; procedural geometry evaluates deterministically; cancellation, reusable functions and supported code/visual equivalence are demonstrated. |
| F. Proven standalone mock workflows | 50–55 | The mock consumes agreed authored data; completion waits, input, live scene inspection, logs and screenshots support a reproducible AI create/run/check/fix workflow without clicks; an equivalent human workflow succeeds; concurrency, stale revisions and instance isolation have regression coverage; mock output remains explicitly simulated. |
| G. Approved runtime integration | 56–65 | Joshua records integration scope and approved access; versioned bridge/capability contracts are agreed; the editor adapter uses approved public APIs; real play/input/logs/screenshots and renderer lifetime/resize work; viewport selection/gizmos/snapping/previews modify the same authoring state; synchronization failures and undo boundaries are tested; the autonomous workflow passes against the real runtime. |
| H. Reproducible game delivery | 66–71 | With an approved runtime/build interface, dependency-aware cooking and incremental builds produce launchable packages from text authoring sources; target settings, cancellation, caching and failures are observable; a sample game runs outside the editor; an AI can create, test and package it without clicks. |
| I. First production editor release | 72–82 | Coherent human workflows and command docs pass private usability testing; keyboard/scaling/contrast/Unicode/accessibility gaps are resolved or explicitly accepted for release; RTX 4070 PC and MSI Claw runs meet agreed budgets; installation/upgrades/recovery/long sessions pass; licences and security boundaries are reviewed; beta blockers are resolved; packages are reproducible and Joshua approves publication. |
| J. Full content suite | 83–93 | Modelling, sculpting, UVs, materials/painting, rigging/skinning and animation workflows use shared commands and text documents; procedural workflows remain editable; dependencies/history/cooking work across content documents; humans and headless agents complete representative workflows; expanded hardware/accessibility/performance/beta/release gates pass. |

## Dependencies and execution policy

- Consolidate the review stack first. Documentation planning can proceed while
  Joshua reviews; runtime feature work should use the agreed reviewed base.
- Next feature is project manifests (step 8), followed by lifecycle (9) and root
  resolution (10). Scope those as separate reviewable slices, not one large PR.
- Object schemas precede component-preserving prefab and asset reference work.
  Project/source identity and dependency rules precede linked prefab updates.
- Graph schemas/commands precede visual editing and procedural evaluation. Mock
  workflow proof precedes claims about autonomous real-game development.
- G is gated on Joshua's explicit integration decision; documenting it grants no
  engine access. H requires the approved runtime/build interface. J follows the
  first production-editor milestone, with its own release validation.
- Windows/CI, usability, accessibility and performance work run alongside feature
  work as infrastructure/hardware becomes available; I is the final release gate.
- Every mutation slice must validate before committing, preserve atomic undo/redo
  and demonstrate rollback on failure. Relevant persistence/parity checks must
  pass; desktop changes need interaction checks and appropriate visual inspection.
- Keep implementation private and small. Joshua merges. CI uses approved
  infrastructure only; paid services and public release need separate approval.
  Artifact uploads remain opt-in. Use only this standalone repository.

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
queries with v1/v2 text persistence.

Local authoring transform commands, numeric forms and JSON v3 persistence now
provide position/rotation/scale data. Read-only world matrix composition now
preserves affine shear and exposes explicit conventions through transform.world.

Conservative world-preserving reparent is now a separate undoable command; shear
and unsafe numerical cases fail without mutation.

Detached prefab subtree export/instantiation now provides reusable authoring
copies through the shared bus. Linked prefab updates/overrides remain future work.

Asset catalogue metadata now has stable project-scoped IDs, portable-style paths,
independent persistence and a command-driven browser. Real file import is deferred.

Registered scene/prefab files can now be validated read-only beneath an explicit
project root, with structured diagnostics and browser/CLI/MCP access.

Registered prefabs can now instantiate by catalogue ID through validated source
resolution, with atomic history and explicit staged-catalogue guards.

Exact-byte fingerprints now support explicit source rechecks and guarded prefab
instantiation. Persistent source tracking/watchers remain future work.

Read-only catalogue-wide source audits now collect deterministic per-ID validation
reports, including explicit unsupported-kind skips.

Full desktop audit tables now expose all returned rows, searchable diagnostics
and source command navigation beyond the bounded generic preview.

Next: consolidate the review stack, then deliver the project manifest and lifecycle
(steps 8–10). See WORK_BOARD.md for bounded tasks and completion evidence. Windows
and target hardware validation remain outstanding. CI is manual/self-hosted and
has not been run.
