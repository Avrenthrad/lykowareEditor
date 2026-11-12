# ADR 0013: Atomic entity duplicate and delete

Status: implemented locally, proposed for Joshua's review.

Add entity.duplicate with explicit source ID and name, and entity.delete with an
explicit ID. Both decode into typed bus mutations and use existing atomic candidate
validation, revision guards and history. Duplicate validates source/name/allocation,
allocates a stable ID and selects it. Delete clears selection only when removing
the selected entity and never rewinds next_id. Undo/redo restores complete state;
failed batches restore selection and allocation as well as scene content.

The current entity consists only of ID/name. Duplication does not clone runtime
state or resources. Richer entity schemas must extend these semantics explicitly.
No scene format change is needed; authoring JSON remains v1.

Desktop context actions target the clicked entity. Duplicate opens a typed name
draft; Delete executes directly with existing undo instead of a blocking dialog.
Palette/shortcut forms prefill selection where available. CLI, MCP and transaction
rows discover the same registrations; no UI-only mutation path is added.

Black-box tests verify selection, atomic undo/redo, monotonic allocation after
deletion, input failures, stale revisions, failed-batch rollback, CLI/MCP parity
and save/load compatibility. Existing desktop interactions remain in the suite.
Windows and richer component/resource duplication remain unverified/future work.
