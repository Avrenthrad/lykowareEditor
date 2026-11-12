# lykowareEditor 1.0 — agent execution board

**Scope decision (2026-11-13):** Version 1.0 targets checklist **1–82**. Checklist **83–93** is a later content-suite release and is excluded from 1.0 acceptance. This planning decision does not authorize engine repository access, CI expenditure, PR merging, or public release.

**Baseline:** The existing checklist reports 1/93 completed (step 2), not an overall completion percentage. No agent work or testing is claimed by creating this board. GitHub-only repositories. Source baseline is `codex/standalone-editor-delivery-milestones`; `main` remains isolated until owner approval. Draft CI PR #1 targets `main` and is not merged.

## Agent work packages

Each agent is a **proposed assignment**, not a running autonomous process. Implement on a dedicated `agent/v1-aNN-<slice>` branch created from the reviewed integration base (or a documented prerequisite branch). Use a separate draft PR for each bounded slice, not one enormous PR per agent. Owners and CI costs require approval. Never self-merge.

| Agent | Scope | Checklist | Dependencies | Exit evidence |
| --- | --- | --- | --- | --- |
| A1 | Foundation, review-stack integration and CI | 1–7 | None for review; owner approval for merges and paid CI | PR #1 merged by owner; reviewed stack; Linux/Windows build+CTest; sanitizers/tidy; fixtures; MCP client tests; numerical budgets |
| A2 | Safe projects and document lifecycle | 8–16 | A1 | Versioned TOML manifests; create/open/close; active roots; dirty-state; safe saves; recovery; migrations; multi-document ownership; cancellation tests |
| A3 | Scene components and object authoring | 17–26 | A1,A2 | Typed schemas; command/UI/MCP parity; multi-edit; copy/group; visibility/lock; validated refs; persistence; scene diagnostics; large-scene benchmarks |
| A4 | Assets, import and linked prefabs | 27–37 | A2,A3 | Portable IDs; import APIs; glTF/materials; previews; dependencies; reimport; safe moves; linked prefab overrides and undo tests |
| A5 | Scripting and procedural graphs | 38–49 | A1,A3 | Licensed runtime decision; command API; scripts; versioned graph schemas; graph editor; validation; deterministic eval; procedural preview; CLI/MCP tests |
| A6 | Mock and autonomous workflow proof | 50–55 | A2,A3,A4,A5 | Mock consumes scene data; deterministic play/inspection; wait conditions; concurrency/isolation; AI and human end-to-end demonstrations |
| A7 | Approved engine bridge and viewport | 56–65 | Owner approval,A3,A4,A6 | ENGINE_NEEDS; versioned bridge; play/input/render; viewport picking/gizmos/snapping; previews; sync; real-runtime autonomous tests |
| A8 | Game build, cooking and packaging | 66–71 | A4,A7 | Reproducible profiles; asset cooking; incremental builds; package/launch commands; sample standalone game; AI package test |
| A9 | Desktop UX, accessibility and hardware | 72–78 | A2–A8 | Onboarding; polish; keyboard docs; accessibility; MSI Claw/RTX4070 validation; budgets; installer/upgrades/recovery |
| A10 | Security, documentation, beta and release | 79–82 | A1–A9 | Licences/security review; tutorials/samples; private beta and regression; reproducible signed/versioned release candidate for owner review |

## Execution order and PR assignment queue

1. **PR-01 (existing):** CI-only draft PR #1; owner reviews/merges; no automatic dispatch.
2. **PR-02 / A1:** Review-stack ancestry and ADR reconciliation; produce an integration branch and draft PR; owner merge gate.
3. **PR-03 / A1:** CI evidence, Windows fixes, fixture projects, MCP client validation, performance-budget definitions. Split into smaller PRs where needed.
4. **PR-04 / A2:** TOML manifest contract, parser, tests (step 8).
5. **PR-05 / A2:** Project lifecycle commands/UI/MCP tests (step 9).
6. **PR-06 / A2:** Active project roots, path traversal tests (step 10).
7. **PR-07 / A2:** Dirty/save/recovery/migrations/multi-document/cancellation, split by checklist step.
8. **PR-08 / A3:** Component schemas, commands and inspector; subsequent small PRs for selection/references/validation.
9. **PR-09 / A4:** Asset identity/import interface; subsequent glTF, dependencies and linked-prefab PRs.
10. **PR-10 / A5:** Scripting ADR and graph schema; subsequent execution/editor/debugging PRs.
11. **PR-11 / A6:** Mock runtime parity and reproducible AI/human demos.
12. **PR-12 / A7:** Engine requirements/bridge contract **planning only** until explicit owner approval; runtime implementation PRs remain blocked.
13. **PR-13 / A8:** Build/cooking/package pipeline **blocked** on approved runtime interface.
14. **PR-14 / A9:** UX/accessibility/hardware validation slices (parallel wherever feasible).
15. **PR-15 / A10:** Security, documentation, beta, release candidate; no publication without approval.

These are **planned PR identifiers**, not existing GitHub PR numbers (except #1). Create real draft PRs only when their code/documentation slices exist.

## Definition of done and lead-agent evaluation

For each PR require: (1) checklist IDs and scope; (2) source branch/base and dependencies; (3) architecture/ADR impact; (4) command parity for desktop/CLI/MCP; (5) validation-before-mutation and atomic undo/redo for edits; (6) explicit file effects and recovery behavior; (7) actual CMake/CTest/sanitizer/Windows evidence, or clearly stated not-run reasons; (8) regressions, security and license review; (9) STATUS and DELIVERY_CHECKLIST updates; (10) independent lead review and owner merge decision. **Do not tick an item without verified full acceptance.**

## Review cadence

Agent submits draft PR and evidence → lead checks diffs, test logs and dependency compatibility → requests fixes or marks review-ready → owner decides merge. Never claim independent agents have started merely because tasks/issues exist. Hosted Actions runs may consume minutes; do not dispatch without separate approval. Owner must approve engine integration access and public release.

## Deferred post-1.0 scope

Steps 83–93: editable geometry/content documents, polygon modelling, sculpting, UV editing, texture painting, rigging/skinning, animation timelines/graphs, cross-document integration, human/headless parity, and expanded validation. Track separately without blocking 1.0.
