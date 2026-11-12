# A1 foundation audit — initial checkpoint (2026-11-13)

Scope: checklist 1–7. **Status: in progress; no checklist step newly completed.** This is a read-only GitHub ancestry/documentation audit, not a fresh build or test run.

## Verified GitHub comparisons

| Base | Head | Ahead | Behind | Changed files |
| --- | --- | ---: | ---: | ---: |
| main | codex/standalone-editor-core | 6 | 0 | 31 |
| codex/standalone-editor-core | codex/standalone-editor-workspace | 2 | 0 | 17 |
| codex/standalone-editor-audit-reports | codex/standalone-editor-delivery-checklist | 1 | 0 | 6 |
| codex/standalone-editor-delivery-checklist | codex/standalone-editor-delivery-milestones | 1 | 0 | 6 |
| codex/standalone-editor-delivery-milestones | codex/validation-checklist-2026-11-12 | 4 | 0 | 3 |

These sampled comparisons support the documented stack structure but **do not prove every intermediate branch is reviewed**, nor that source code builds. The validation branch adds CI and documentation changes and is not the reviewed integration baseline.

## Existing evidence and limitations

- REVIEW_STACK.md documents 26 ordered development branches. It reports 34 desktop and 26 sanitizer/headless test groups passed at an earlier Linux checkpoint; these were not rerun in this audit.
- Windows builds, real MCP-client interoperability, target hardware and fresh hosted CI remain unverified.
- The existing CI-only PR #1 is draft/unmerged and must be reviewed/merged by the owner before dispatch is available from main.
- The 1.0 task board lives in docs/V1_AGENT_BOARD.md on planning/v1-agent-task-board (draft PR #13). 1.0 includes steps 1–82; 83–93 are deferred.
- Existing review-stack documentation includes outdated statements that GitHub API PR creation is blocked and that CI is self-hosted; later GitHub PR creation and hosted-runner workflow preparation supersede those statements. Reconcile documentation in a separate scoped PR after review.

## A1 next actions and acceptance gates

1. Compare **every adjacent pair** in REVIEW_STACK.md; confirm no divergence, unintended files or gaps.
2. Inspect ADRs and CMake preset/workflow contracts for inconsistencies; record concrete findings.
3. Prepare an integration branch from the verified final code checkpoint, keeping main untouched.
4. Review CI-only PR #1. **Do not merge** without owner approval.
5. After explicit approval of CI expenditure/usage, dispatch Linux/Windows jobs against the source-bearing branch; capture logs, versions and test counts. Until then, status is **not run**.
6. Create representative small/large/malformed/migration fixture projects and deterministic tests.
7. Validate real MCP clients, instance isolation and define numeric performance budgets.
8. Open separate draft fix PRs for reproducible failures; review each independently.

## Safety

GitHub only. No automatic merges, hosted CI dispatch, paid services, engine repository access or release publication. Keep all implementation slices isolated. For each checklist item, require actual acceptance evidence before ticking.

## CI authorisation and preflight (2026-11-13)

Owner authorised **manual hosted CI within existing GitHub Actions usage limits**, **conditional on separately approving merge of CI PR #1**. This is not merge approval. No run has been dispatched.

- Verified source branch `codex/standalone-editor-delivery-milestones` has `editor/CMakePresets.json` with `dev`, `desktop`, `sanitizers`, and `thread-sanitizer` configure/build/test presets. CI references these names.
- Manual workflow in CI PR #1 should be reviewed for workflow syntax, Linux dependencies, Windows generator/toolchain and vendor downloads. Build/test success is **not** established by preset existence.
- Before dispatch: confirm GitHub Actions enabled, billing/minute balance and repo permissions; obtain separate approval to merge PR #1; select a branch that actually contains both editor sources and workflow. Prefer `codex/validation-checklist-2026-11-12` after verifying its current contents.
- In GitHub UI: Actions → Standalone editor checks → Run workflow → choose validated source branch → Run workflow. **Do not select `main`**, which lacks editor sources. Record run URL, job results, OS versions, CTest totals, sanitizers/tidy diagnostics and any billable-minute consumption.
- Manual CI approval does not authorise agent-initiated merges or paid overage. Stop before dispatch if usage limits cannot be confirmed.

### Audit limitation

Attempted to batch all 26 adjacency comparisons, but the connector orchestration reached its tool-call limit before returning results. No comprehensive ancestry conclusion is claimed. The five earlier sampled comparisons remain the only recorded ancestry evidence in this report.
