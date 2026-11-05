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

Draft PR creation is currently blocked by the environment policy for
api.github.com (CONNECT 403). Git fetch/push works through the existing connection.
The required API hostname has been added to the environment configuration draft;
it has not been applied to the running policy. API authentication remains untested.
No token values are requested or stored. After relevant settings are applied,
retry the API check before requesting credentials or creating duplicate PRs.

Local validation: dev preset 10/10, desktop preset 13/13, ASan/UBSan preset 10/10 with
local leak-check limitation, queue ThreadSanitizer check, clang-tidy with no
project-code findings, and X11 interaction/capture inspection. Windows and remote
CI remain unrun. Joshua reviews and merges.
