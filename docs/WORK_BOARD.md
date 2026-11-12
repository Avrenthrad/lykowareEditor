# Editor work board

Last updated: 2026-11-07. Work only in this repository; Joshua reviews and merges.

[Delivery checklist](DELIVERY_CHECKLIST.md) · [Milestones and acceptance criteria](ROADMAP.md)
· [Implementation status](STATUS.md) · [Private review stack](REVIEW_STACK.md)

## Workflow

Use stable checklist IDs in tasks and PR descriptions. States: Ready, In progress,
Implemented (awaiting review), Blocked, Done. A tested local implementation can tick
its checklist item while awaiting review; record that distinction. Done on this
board means Joshua's review/merge is recorded, not merely that a branch was pushed.
Keep only one active implementation slice here unless explicitly coordinated.

Before starting: read STATUS and the checklist, verify checkout/base and local
changes, identify the task's acceptance criteria, and record the active slice.
After finishing: record evidence/commit/branch, update checklist counts and STATUS,
and move to Implemented. Never mark other people's review or hardware work done
without evidence. Reopen tasks when regressions invalidate their acceptance.
Later work remains in the checklist until ready to be split; this board is the
near-term queue, not a second copy of all 93 steps.

## Active queue

| Task | Checklist ID | State | Dependencies / completion evidence |
| --- | --- | --- | --- |
| Milestones, acceptance criteria and maintained board | 2 | Implemented (awaiting review) | ROADMAP defines all ten milestone ranges, observable exit criteria and dependencies; this board defines states, completion policy and bounded next slices. Checklist/links/coverage checked. Private branch: codex/standalone-editor-delivery-milestones. |
| Review foundation stack and proposed ADRs | 1 | Ready | Start at REVIEW_STACK in base order; record findings and fixes against immutable commits; Joshua records review/merge decisions. Do not merge. GitHub API policy currently prevents automated PR creation; private branches and the local review index remain available. |
| Representative fixture projects | 3 | Implemented (awaiting review) | editor/tests/fixtures covers all current format versions, malformed candidates and generated 1000-entity/4096-asset workloads; fixture_projects passes on desktop and ASan/UBSan. Branch codex/standalone-editor-project-fixtures. Extend for future formats. |
| Numerical performance budgets | 7 | Implemented (awaiting review) | PERFORMANCE_BUDGETS defines numerical proposed targets and workload/measurement boundaries; opt-in benchmark-headless.py ran successfully in Debug/Release. Hardware acceptance remains steps 76–77. Branch codex/standalone-editor-performance-budgets. |
| Windows compilation and runtime validation | 4 | Blocked | Requires a Windows environment; no successful Windows result is recorded. Do not infer success from Linux or CMake portability. |
| Approved cross-platform CI execution | 5 | Blocked | Requires approved available runners. Existing manual CI configuration is not evidence of a run. No paid jobs or artifact storage without Joshua's approval. |
| Real MCP client interoperability | 6 | Ready | Select available actual clients; record versions and results for negotiation/tools/errors, shared-instance concurrency and instance isolation. Existing transport tests alone do not complete this task. |
| Project manifest contract and ADR | 8a (part of 8) | Implemented (awaiting review) | ADR 0025 and project.manifest.validate/write implemented on codex/standalone-editor-project-manifest. Defines TOML version, project identity/name, root and relative directory conventions, defaults, unknown-field policy, validation, migration boundary and file-effect semantics. No runtime/engine dependency. |
| Project manifest model and bounded persistence | 8b (part of 8) | Implemented (awaiting review) | Implemented model, bounded parser and no-overwrite writer with shared commands. Uses existing helpers without a new dependency. Planned acceptance: implement value model/parser/writer using existing wrappers or an approved pinned permissive dependency. Round-trip valid Unicode/settings; reject malformed/unsupported/path-invalid/oversized data without state changes; protect existing files according to the agreed save contract. |
| Manifest tests and documentation | 8c (part of 8) | Implemented (awaiting review) | Python tomllib interoperability, direct typed-model tests and CLI/MCP parity pass. Full validation evidence is in STATUS.md. Planned acceptance: verify round trips, deterministic output, invalid candidate rejection and filesystem errors on available platforms. Record remaining Windows limits; update protocol/docs/STATUS/checklist. Tick step 8 only after all its agreed requirements are met. |
| Project lifecycle commands | 9 | Implemented (awaiting review) | Context (9a), creation (9b), and explicit persistent recent history (9c) implemented. Full 41 desktop / 32 sanitizer checks and native X11 navigation pass. Safe in-place saves and dirty-state tracking remain separate steps 11–12. |
| Project context, conservative guards and session-local recents | 9a (part of 9) | Implemented (awaiting review) | ADR 0026; shared open/close/query/recent.clear commands, atomic history/candidate protection, canonical MRU metadata and Project-menu form navigation. Branch codex/standalone-editor-project-context. Validation in STATUS.md. |
| Complete project creation workflow | 9b (part of 9) | Implemented (awaiting review) | ADR 0027: supplied or generated identity, new-root reservation, configured folders, manifest-last publication and cleanup. Project menu/create-result navigation shares commands. Full 40/31 checks plus targeted generation tests; branch codex/standalone-editor-project-creation. |
| Persistent user-local recent settings | 9c (part of 9) | Implemented (awaiting review) | ADR 0028: user-local versioned JSON session history, explicit no-overwrite export, undoable import, stale metadata without activation and no portable-format path leakage. Branch codex/standalone-editor-project-recents. |
| Active-project asset root resolution | 10 | Implemented (awaiting review) | ADR 0029; optional/blank roots use committed active project, concrete result roots freeze follow-up forms, implicit prefab origins guard staged context/catalogue changes. Full 42/33 checks pass; branch codex/standalone-editor-active-roots. |
| Dirty-state model and document baselines | 11a (part of 11) | Implemented (awaiting review) | Define document ownership, clean baselines, edit/save/load/undo/redo behavior and project-settings scope. Track actual persisted authoring data rather than every UI/runtime revision; failed operations never clear dirty state. |
| Dirty-state commands and desktop feedback | 11b (part of 11) | Implemented (awaiting review) | Requires 11a. Expose coherent document status through the shared bus and UI; validate scene/catalogue/project-settings workflows and retain lifecycle content protection until dirty-aware switching is explicitly designed. |

Core review R001 fixed on 98b9732; the full review and Joshua's decisions remain
open. Manifest (8), fixture projects (3) and numerical budgets (7) are implemented
locally. Context, creation and persistent recent history complete the explicit
lifecycle workflows in step 9. Active-project root resolution (10) is implemented;
dirty-state tracking (11) is next. Private stacked work
continues at the user's request without asserting review approval or merging.
Step 2's completion does not imply milestone A is complete or that code is merged.

## Decision and access gates

- Engine integration (56–65): planning only until Joshua decides scope/access;
  requirements go into ENGINE_NEEDS.md. Do not access another repository.
- Real game packaging (66–71): requires approved runtime/build capabilities.
- Hardware/accessibility/performance claims: require recorded target measurements.
- Publication/paid infrastructure: require explicit approval. No automatic action.

## Update record

- 2026-11-06: Established milestone exit criteria and this active queue. Checklist
  step 2 implemented locally; review/merge remains with Joshua. Existing tests are
  the prior code checkpoint, not new validation of unimplemented feature tasks.

- 2026-11-06: Manifest and fixture slices implemented; defined numerical budgets
  with a measured local Release/mock baseline. No target hardware pass is claimed.

- 2026-11-06: Project context slice 9a implemented locally with native/CLI/MCP
  validation. Step 9 stays partial; checklist count remains 4/93.

- 2026-11-07: Lifecycle step 9 implemented locally across context/creation/recent
  slices. Full 41/32 checks pass; count 5/93. No merge or platform claims are implied.

- 2026-11-07: Step 10 implemented with full 42 desktop / 33 sanitizer checks;
  checklist 6/93. Next active queue is dirty-state design and implementation.

Active request: implement checklist 11–20; see ACTIVE_BATCH.md. Step 11 is locally
implemented with full 43/34 checks. Step 12 checked persistence is in progress.

Step 12 checked saves are implemented locally: full 44/35 checks, ADR 0031, branch
codex/standalone-editor-checked-saves. Continue batch with recovery/autosave (13).
