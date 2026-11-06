# Editor work board

Last updated: 2026-11-06. Work only in this repository; Joshua reviews and merges.

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
| Representative fixture projects | 3 | Ready | Inventory existing test-generated examples; add reusable small/large/malformed/migration fixtures with reproducible expectations. Full completion requires supported formats and migration coverage, not only today's scenes. |
| Numerical performance budgets | 7 | Ready | Specify representative workloads, agreed numerical targets and repeatable measurement methods. Distinguish local software/mock measurements from RTX 4070/MSI Claw results. |
| Windows compilation and runtime validation | 4 | Blocked | Requires a Windows environment; no successful Windows result is recorded. Do not infer success from Linux or CMake portability. |
| Approved cross-platform CI execution | 5 | Blocked | Requires approved available runners. Existing manual CI configuration is not evidence of a run. No paid jobs or artifact storage without Joshua's approval. |
| Real MCP client interoperability | 6 | Ready | Select available actual clients; record versions and results for negotiation/tools/errors, shared-instance concurrency and instance isolation. Existing transport tests alone do not complete this task. |
| Project manifest contract and ADR | 8a (part of 8) | Implemented (awaiting review) | ADR 0025 and project.manifest.validate/write implemented on codex/standalone-editor-project-manifest. Defines TOML version, project identity/name, root and relative directory conventions, defaults, unknown-field policy, validation, migration boundary and file-effect semantics. No runtime/engine dependency. |
| Project manifest model and bounded persistence | 8b (part of 8) | Implemented (awaiting review) | Implemented model, bounded parser and no-overwrite writer with shared commands. Uses existing helpers without a new dependency. Planned acceptance: implement value model/parser/writer using existing wrappers or an approved pinned permissive dependency. Round-trip valid Unicode/settings; reject malformed/unsupported/path-invalid/oversized data without state changes; protect existing files according to the agreed save contract. |
| Manifest tests and documentation | 8c (part of 8) | Implemented (awaiting review) | Python tomllib interoperability, direct typed-model tests and CLI/MCP parity pass. Full validation evidence is in STATUS.md. Planned acceptance: verify round trips, deterministic output, invalid candidate rejection and filesystem errors on available platforms. Record remaining Windows limits; update protocol/docs/STATUS/checklist. Tick step 8 only after all its agreed requirements are met. |
| Project lifecycle commands | 9 | Blocked | Requires 8. Specify create/open/close/recent-project state and atomic boundaries, then implement shared UI/CLI/MCP access. Reject invalid projects without replacing current state; do not silently discard dirty work. |
| Active-project asset root resolution | 10 | Blocked | Requires 9. Resolve source operations against the active project; preserve validation, revision and fingerprint guards; document explicit-root compatibility; verify frontend parity and no active-project error behavior. |

Core review R001 fixed on 98b9732; the full review and Joshua's decisions remain
open. Manifest slice 8 is implemented locally; next independent work is reusable
fixture projects (3), followed by project lifecycle (9). Private stacked work
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
