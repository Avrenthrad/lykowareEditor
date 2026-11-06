# Private editor review stack

All branches are in this repository. Nothing has been merged. Review from top to
bottom; each branch includes its base. No hosted CI or artifact upload was run.

| Branch | PR base | Scope |
| --- | --- | --- |
| codex/standalone-editor-core | main | Architecture, typed transactions, CLI/MCP, scenes and mock feedback |
| codex/standalone-editor-workspace | codex/standalone-editor-core | Tokens, workspace, selection, keymap, palette and TOML config |
| codex/standalone-editor-async | codex/standalone-editor-workspace | Bounded desktop execution, history and coherent snapshots |
| codex/standalone-editor-vendor | codex/standalone-editor-async | Pinned licensed ImGui/SDL source and offline wrapper |
| codex/standalone-editor-desktop | codex/standalone-editor-vendor | Window/panels, shared MCP, input tests and rootless setup |
| codex/standalone-editor-forms | codex/standalone-editor-desktop | Registry metadata, typed palette forms, contextual defaults and form interaction tests |
| codex/standalone-editor-docking | codex/standalone-editor-forms | Validated layout trees, TOML v2 migration and undoable native docking |
| codex/standalone-editor-transactions | codex/standalone-editor-docking | Typed atomic batch builder, row validation and desktop interaction tests |
| codex/standalone-editor-shortcuts | codex/standalone-editor-transactions | Searchable shortcut browser, binding edit links, live action hints and rebinding interaction tests |
| codex/standalone-editor-layout-forms | codex/standalone-editor-shortcuts | Typed layout object drafts, JSON import gating and model/native interaction tests |
| codex/standalone-editor-visual-polish | codex/standalone-editor-layout-forms | Embedded Apache-2.0 Roboto, original vector icons and token-driven visual polish |
| codex/standalone-editor-form-feedback | codex/standalone-editor-visual-polish | Ticket-correlated inline feedback, field errors, bounded results and correction/retry tests |
| codex/standalone-editor-entity-actions | codex/standalone-editor-form-feedback | Atomic entity duplicate/delete, contextual actions and CLI/MCP authoring tests |
| codex/standalone-editor-scene-tree | codex/standalone-editor-entity-actions | Validated authoring hierarchy, tree queries and v1/v2 scene persistence |
| codex/standalone-editor-transforms | codex/standalone-editor-scene-tree | Local transform commands, numeric forms and v3 scene persistence |
| codex/standalone-editor-world-transforms | codex/standalone-editor-transforms | Read-only world matrices, explicit conventions and analytic CLI/MCP tests |
| codex/standalone-editor-preserve-world | codex/standalone-editor-world-transforms | Conservative atomic world-preserving reparent and analytic rejection/history tests |
| codex/standalone-editor-prefabs | codex/standalone-editor-preserve-world | Detached prefab subtree export, atomic instantiation and CLI/MCP persistence tests |
| codex/standalone-editor-assets | codex/standalone-editor-prefabs | Stable catalogue metadata, validated relative paths, independent persistence and browser |
| codex/standalone-editor-source-validation | codex/standalone-editor-assets | Read-only project-root scene/prefab validation and structured source diagnostics |
| codex/standalone-editor-asset-instantiation | codex/standalone-editor-source-validation | Registered prefab instantiation, captured source/candidate metadata guards and browser action |
| codex/standalone-editor-source-fingerprints | codex/standalone-editor-asset-instantiation | Exact-byte SHA-256, source comparisons and checked instantiation |
| codex/standalone-editor-source-audit | codex/standalone-editor-source-fingerprints | Read-only catalogue-wide source reports, explicit skips and desktop summary |
| codex/standalone-editor-audit-reports | codex/standalone-editor-source-audit | Full searchable audit report table and guarded source form navigation |
| codex/standalone-editor-delivery-checklist | codex/standalone-editor-audit-reports | Maintained 93-step delivery checklist, prominent links and handover instructions |

Draft PR creation is currently blocked by the environment policy for
api.github.com (CONNECT 403). Git fetch/push works through the existing connection.
The required API hostname has been added to the environment configuration draft;
it has not been applied to the running policy. API authentication remains untested.
No token values are requested or stored. After relevant settings are applied,
retry the API check before requesting credentials or creating duplicate PRs.

Local validation: desktop preset 34/34 (including 26 headless/model groups),
ASan/UBSan preset 26/26 with
local leak-check limitation, queue ThreadSanitizer check, clang-tidy with no
project-code findings, and X11 interaction/capture inspection of the audit report
diagnostic view. Windows and remote
CI remain unrun. Joshua reviews and merges.
