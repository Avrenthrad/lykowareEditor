# Lykoware Editor delivery checklist

**Project reference — linked at the top of README and ROADMAP.**

Last updated: 2026-11-06. Owner/reviewer: Joshua, Lykoleon Studios.

## Current position

- Foundation already implemented: shared command bus, atomic history, headless CLI,
  instance-targeted MCP, desktop shell, hierarchy/transforms, detached prefabs,
  catalogue/source validation and full desktop audit reports.
- Latest code checkpoint: `2bfbccd`; 34 desktop test groups and 26 ASan/UBSan
  headless groups passed. See [STATUS](STATUS.md) for evidence and limitations.
- **Remaining checklist: 1/93 complete.** This counts the remaining delivery steps,
  not overall project completion. Existing foundations are not counted again.
- Step 2 is implemented locally: [milestone acceptance criteria](ROADMAP.md) and
  the [active work board](WORK_BOARD.md) are maintained alongside this checklist.
  Review/merge remains with Joshua; the wider foundation milestone is incomplete.
- Recommended next feature: step 8, project manifest, after review-stack
  consolidation. Windows/CI validation should progress as infrastructure permits.
- Steps 56–65 require Joshua's integration decision. This plan does not authorize
  engine repository access. Steps 83–93 are the later content-suite finish line.

## Maintenance and completion rules

Update this file and STATUS.md after each task. Keep the numbered IDs stable.
Tick an item only when its whole stated outcome is implemented and appropriately
verified; record evidence, commit/PR references and remaining limits below it.
Partial work stays unchecked with a short progress note. Track implementation,
review and merge separately: a tick does not claim Joshua has merged a branch.
Reopen items if later evidence invalidates completion. Update the count and current
position together. Add newly discovered work as subtask notes or appended IDs;
do not silently renumber the original 93 steps. No automatic background updates
are implied: maintain this checklist during each working session.

For every implementation slice: shared commands; validate before mutation; atomic
undo/redo for editor changes; explicit external file effects; appropriate checks;
current docs; small private review branches. Joshua merges. No paid services or
public release without approval. See [review stack](REVIEW_STACK.md).

## A. Consolidate the foundation — steps 1–7

- [ ] 1. Review the existing branch stack and ADRs; resolve issues before building further on them. Joshua reviews and merges.
- [x] 2. Replace the short roadmap with milestones, acceptance criteria and a maintained work board.
  Evidence: ROADMAP.md covers milestones A–J with observable exit criteria and dependencies; WORK_BOARD.md defines the queue, task states and next slices. Documentation links, stable IDs and milestone coverage checked. Implemented locally on codex/standalone-editor-delivery-milestones; awaiting Joshua review/merge.
- [ ] 3. Establish representative test projects: small, large, malformed and migration fixtures.
- [ ] 4. Verify Windows builds and fix platform-specific problems.
- [ ] 5. Run Linux/Windows CI on approved infrastructure, including sanitizers and clang-tidy.
- [ ] 6. Validate compatibility with actual MCP clients and multiple editor instances.
- [ ] 7. Define measurable performance budgets for startup, frame time, memory and command latency.

## B. Make projects safe and practical — steps 8–16

- [ ] 8. Add a versioned TOML project manifest with project identity, settings and directory conventions.
- [ ] 9. Add project creation, opening, closing and recent-project workflows through shared commands.
- [ ] 10. Resolve asset paths against the active project so users need not repeatedly enter its root.
- [ ] 11. Add dirty-state tracking for scenes, project settings and other editable documents.
- [ ] 12. Support safe saving to existing files, including external-change detection and conflict handling.
- [ ] 13. Add autosave, recovery snapshots and crash-recovery workflows.
- [ ] 14. Add schema migrations with backups and clear compatibility errors.
- [ ] 15. Support opening multiple documents with clear active-document and history ownership.
- [ ] 16. Add progress and cancellation for long operations while preserving atomic commits.

## C. Complete scene and object authoring — steps 17–26

- [ ] 17. Define a typed, extensible component/property schema.
- [ ] 18. Add component creation, removal, inspection and property editing commands.
- [ ] 19. Generate inspector controls from those schemas, including nested objects, arrays and references.
- [ ] 20. Add multi-selection and atomic operations across selected entities.
- [ ] 21. Add copy/paste, grouping and reusable selection workflows.
- [ ] 22. Add visibility, locking and other agreed authoring metadata.
- [ ] 23. Add validated references between entities and assets.
- [ ] 24. Preserve components and references through save/load, duplication and prefab instantiation.
- [ ] 25. Add scene-wide validation with actionable diagnostics.
- [ ] 26. Benchmark large scenes and improve data/history storage where measurements require it.

## D. Turn the asset catalogue into an asset pipeline — steps 27–37

- [ ] 27. Define portable asset identities and reference rules beyond catalogue-local numeric IDs.
- [ ] 28. Add explicit asset discovery and registration workflows.
- [ ] 29. Define importer interfaces, versioned import settings and derived-data storage.
- [ ] 30. Add cancellable import jobs with progress and diagnostics.
- [ ] 31. Implement initial glTF mesh import and supported material/texture handling.
- [ ] 32. Add previews and metadata for supported asset kinds.
- [ ] 33. Track asset dependencies and identify missing or broken references.
- [ ] 34. Detect changed source files and support explicit reimport.
- [ ] 35. Make asset moves, renames and deletion update or diagnose affected references.
- [ ] 36. Add linked prefab definitions, instances and override tracking.
- [ ] 37. Add apply/revert/update workflows for prefab instances with atomic undo.

## E. Deliver scripting and procedural authoring — steps 38–49

- [ ] 38. Choose and document a permissively licensed scripting approach.
- [ ] 39. Expose the shared command registry to scripts.
- [ ] 40. Add script editing, validation, diagnostics and execution workflows.
- [ ] 41. Define versioned text schemas for node graphs, nodes, ports and connections.
- [ ] 42. Add transactional graph editing commands.
- [ ] 43. Build the visual graph editor over those commands.
- [ ] 44. Validate types, connections, cycles and execution rules.
- [ ] 45. Implement deterministic graph evaluation with cancellation and caching where appropriate.
- [ ] 46. Add procedural geometry operations and a software/mock preview.
- [ ] 47. Ensure graphs can be created, inspected and edited entirely through text, CLI and MCP.
- [ ] 48. Define reusable graph functions and the agreed relationship between visual scripting and code.
- [ ] 49. Add debugging tools for scripts and graphs: errors, execution inspection and reproducible tests.

## F. Prove the complete workflow against the mock — steps 50–55

- [ ] 50. Make the mock consume the agreed scene/component data so previews reflect authored content.
- [ ] 51. Strengthen deterministic play/stop, input injection, logs, screenshots and live-scene inspection.
- [ ] 52. Add observable completion and reliable wait conditions for automated workflows.
- [ ] 53. Test concurrent clients, stale revisions, instance isolation and failure recovery.
- [ ] 54. Demonstrate an AI creating, saving, running, inspecting and correcting a sample project without clicks.
- [ ] 55. Demonstrate a human completing the equivalent workflow comfortably through the UI.

## G. Engine integration decision and editor adapter — steps 56–65

Planning only until Joshua decides integration scope and access. Current work stays
standalone in this repository. Record requirements in [ENGINE_NEEDS](ENGINE_NEEDS.md).

- [ ] 56. Finalize the editor's requirements in docs/ENGINE_NEEDS.md.
- [ ] 57. Agree on a versioned EngineBridge contract and capability negotiation.
- [ ] 58. Build the editor-side adapter against the approved public interface.
- [ ] 59. Integrate real play mode, input, logs, screenshots and runtime scene queries.
- [ ] 60. Integrate the approved rendering surface and its resize/lifetime behavior.
- [ ] 61. Add viewport navigation, picking, selection outlines and transform gizmos.
- [ ] 62. Add snapping, coordinate-space controls and useful scene visualization.
- [ ] 63. Add renderer-backed asset, material and lighting previews.
- [ ] 64. Verify authoring/runtime synchronization, including failures and undo boundaries.
- [ ] 65. Repeat the autonomous creation-and-correction workflow against the real runtime.

## H. Make building and shipping a game real — steps 66–71

Real game builds depend on an approved runtime/build interface; mock packaging
alone does not satisfy these steps.

- [ ] 66. Define reproducible build profiles, target settings and capability checks.
- [ ] 67. Implement dependency-aware asset cooking while retaining text authoring sources.
- [ ] 68. Add incremental builds, caching, cancellation and structured build diagnostics.
- [ ] 69. Add packaging and launch commands through the shared registry.
- [ ] 70. Produce and test a standalone sample game outside the editor.
- [ ] 71. Demonstrate an AI creating, testing and packaging that game without clicks.

## I. Finish the human experience and first production release — steps 72–82

These are release gates, not a reason to defer usability or validation until the end.

- [ ] 72. Complete coherent workflows for onboarding, projects, scenes, assets, scripting and builds.
- [ ] 73. Polish typography, icons, spacing, motion, empty states and error recovery.
- [ ] 74. Complete keyboard navigation, shortcuts, contextual menus and command documentation.
- [ ] 75. Validate scaling, contrast, Unicode coverage and accessibility; address toolkit limitations.
- [ ] 76. Test touch/controller usability on the MSI Claw and desktop use on the RTX 4070 PC.
- [ ] 77. Measure frame times, responsiveness and memory on target hardware; fix missed budgets.
- [ ] 78. Test installation, upgrades, recovery and representative long editing sessions.
- [ ] 79. Review dependency licences, distribution notices and security-sensitive boundaries.
- [ ] 80. Finish tutorials, reference documentation and human/AI sample projects.
- [ ] 81. Run a private beta, resolve release-blocking feedback and complete regression testing.
- [ ] 82. Prepare versioned release packages for Joshua's review. Publish only after approval.

## J. Complete the later content-suite vision — steps 83–93

- [ ] 83. Extend the object and command schemas for editable geometry and content documents.
- [ ] 84. Build polygon modelling tools and procedural modelling workflows.
- [ ] 85. Add sculpting tools.
- [ ] 86. Add UV editing.
- [ ] 87. Add texture painting and material authoring.
- [ ] 88. Add rigging, skinning and weight editing.
- [ ] 89. Add animation timelines, curves and keyframe editing.
- [ ] 90. Add animation graphs and preview workflows.
- [ ] 91. Integrate these documents with assets, dependencies, undo, persistence and cooking.
- [ ] 92. Prove every content workflow through both human controls and headless AI commands.
- [ ] 93. Repeat hardware, accessibility, performance, beta and release validation for the expanded suite.

## Change record

- 2026-11-06: Saved the agreed 93-step remaining-delivery plan, established progress
  rules and linked it prominently for quick reference. Step 2 is partial; no full
  remaining step is claimed complete by this documentation task.

- 2026-11-06: Completed step 2 locally: milestone acceptance criteria, dependency
  gates and an active work board. Count is now 1/93; this is not an effort-weighted
  project percentage. Review-stack consolidation remains open.

## 2026-11-12 remote validation checkpoint (non-executable)

- Scope: GitHub repository/branch/tree inspection and CMake/CI configuration review. **No fresh compilation, CTest run, desktop interaction, sanitizer execution, Windows validation or hardware benchmark was performed.** The execution environment could not resolve github.com for a clone. Do not interpret this as a passing build.
- Default `main` still contains only `README.md` at the initial commit. The working editor source is isolated on the `codex/standalone-editor-*` review branches; no open pull requests were found. The delivery-milestones branch includes the C++20 source, desktop adapter, tests and documentation.
- The desktop and headless test targets are defined in `editor/CMakeLists.txt`; `.github/workflows/editor.yml` is manually dispatched on self-hosted Linux/Windows runners. Test definitions are not evidence that tests passed on this date.
- Prior recorded checkpoint (2026-11-06): 34 desktop and 26 sanitizer/headless test groups passed locally on Linux, with leak detection limited. Those are historical claims from STATUS/REVIEW_STACK, not independently reproduced here.
- Outstanding verification gates: review stack ancestry and integration (step 1), reusable fixtures (3), Windows build (4), approved CI execution (5), real MCP clients (6), numerical performance budgets (7). Do not mark any complete on the strength of this read-only inspection.
- **Checklist remains 1/93 complete**, with step 2 implemented but awaiting owner review/merge. Next: obtain an executable checkout or approved runner, run `cmake --preset dev && cmake --build --preset dev && ctest --preset dev` and the desktop/sanitizer presets from `editor/`, record command output and environment, then resolve failures before integrating the stack. Do not merge or publish automatically.

## 2026-11-12 hosted Actions preparation

- Updated `.github/workflows/editor.yml` on this review branch to use standard `ubuntu-24.04` and `windows-2022` hosted runners, retaining **only** `workflow_dispatch`, read-only permissions, a single concurrent matrix leg, 45-minute per-job timeouts, and no artifact uploads. This is preparation, **not** a completed CI validation.
- No workflow was dispatched and no GitHub-hosted usage was incurred by this change. GitHub-hosted runner minutes and billing depend on repository visibility and account allowances; verify Actions budget before running.
- The default branch currently lacks this workflow. For the GitHub Actions UI/manual dispatch to be available, the workflow must first exist on the default branch (e.g. through an owner-approved integration); do not merge the editor stack automatically. Alternatively use a temporary, owner-approved default-branch workflow after reviewing the approach.
- Step 5 remains open until real Ubuntu/Windows run URLs, job results, and logs are captured; steps 4 and 7 remain open pending Windows and performance evidence. **Total remains 1/93**.
