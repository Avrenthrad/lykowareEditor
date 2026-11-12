# ADR 0021: Registered prefab instantiation

Status: implemented locally, proposed for Joshua's review.

Add asset.instantiate(id, root, parent) as a mutation in the shared registry. Resolve
ID against the committed catalogue, require prefab kind, and reuse the source
reader from asset.validate for explicit root, observed canonical containment,
regular-file and bounded JSON checks. Parse the prefab once and pass captured
authoring data plus origin ID/path/kind to the existing InstantiatePrefab mutation.
The pure bus performs no file I/O. File-based instantiation keeps its previous API.

Check origin metadata against the transaction candidate before allocation. If an
earlier row removes/changes the entry, reject as asset_reference_changed/id rather
than silently applying a source resolved from older metadata. New registration
must commit before ID-based resolution; staged destination parents are supported.
Explicit-path prefab.instantiate remains usable for atomic registration-plus-copy
workflows. Revision guards run before source decoding. Failed batches roll back
selection, IDs, hierarchy, transforms and catalogue changes.

Reuse the existing instantiation/history implementation. Redo restores captured
authoring state even if files later change/disappear. Origin metadata is only a
transient guard, not a persisted prefab link or asset reference. No file writes,
resource caches, runtime objects or persistence schema changes are introduced.
The source reader retains its documented normal-project-tree limitations; this
adds no race-proof filesystem sandbox or file-identity guarantee.

Expose a prefab-only browser action that opens the shared ID/root/parent form.
Use two action lines per row to avoid crowding. Tests cover ID/type/source errors,
candidate catalogue guards, staged parents, rollback, stale revision precedence,
redo after deletion and CLI/MCP parity. Linked definitions, source fingerprints,
global GUIDs and runtime integration remain future work.
