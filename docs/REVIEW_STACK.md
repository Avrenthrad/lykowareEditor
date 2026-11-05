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

Draft PR creation is currently blocked by the environment policy for
api.github.com (CONNECT 403). Git fetch/push works through the existing connection.
The required API hostname has been added to the environment configuration draft;
it has not been applied to the running policy. API authentication remains untested.
No token values are requested or stored. After relevant settings are applied,
retry the API check before requesting credentials or creating duplicate PRs.

Local validation: desktop preset 19/19 (including 13 headless/model groups),
ASan/UBSan preset 13/13 with
local leak-check limitation, queue ThreadSanitizer check, clang-tidy with no
project-code findings, and X11 interaction/capture inspection. Windows and remote
CI remain unrun. Joshua reviews and merges.
