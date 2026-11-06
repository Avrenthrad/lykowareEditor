# ADR 0029: Active-project source root resolution

Status: implemented locally, proposed for Joshua's review. Checklist step 10.

For asset.validate, asset.check, asset.audit, asset.instantiate and
asset.instantiate_checked, make root an optional string. Omitted/empty root resolves
the committed active ProjectSession.root; no active context returns
no_active_project/root. A nonempty explicit root retains prior semantics and never
falls back when invalid. Registry descriptors declare the optional field, empty
default and explanatory text; all other fields retain their existing requirements.
The generated desktop forms submit the empty default, and CLI/MCP may omit it.
Do not prefix catalogue paths with the manifest's asset-directory setting: paths
remain relative to the project root, including scenes/scripts/graphs/etc.

Resolve from a coherent committed document while the command service serializes
calls. Existing target/revision/schema/identity/kind/fingerprint validation and
bounded/canonical source checks remain in force. A newly staged project.open does
not establish an implicit root until committed. Explicit overrides remain available
for deliberate independent-source use, including staged authoring transactions.

Capture the source ProjectSession on implicitly resolved prefab mutations. The
pure bus compares it to candidate active context before allocation, in addition to
the existing catalogue-origin check. A transaction can otherwise clear the
catalogue, switch context and recreate identical catalogue IDs/path/kind while
consuming bytes read from the old root. Reject as project_context_changed/root
without committing any staged changes. Even metadata-only context changes require
an explicit commit before resolving implicit source roots. Explicit-root mutations
have no project-origin guard, retaining their intentional override behavior.

Validation/check responses add root: the canonical UTF-8 directory captured by the
same bounded source read used for parsing and hashing. Audit reports add one
canonical root at report level, not redundant per-row roots. Follow-up Recheck,
Instantiate-validated-bytes and audit-row forms prefill this concrete observed root
rather than reinterpreting an old empty field against a later active project.
Desktop validation feedback shows the resolved root. Queries remain read-only;
scene/catalogue/workspace schemas and runtime integration are unchanged.

Normal filesystem race limits remain; neither roots nor hashes are file locks.
Redo restores captured authoring state without source reads. Tests cover implicit
and explicit reads/copies/checks/audits, no-active errors, malformed root types,
schema discovery, staged opens/context switches with identical catalogue origins,
existing catalogue guards, rollback and CLI/MCP parity. Windows and target hardware
remain unverified.
