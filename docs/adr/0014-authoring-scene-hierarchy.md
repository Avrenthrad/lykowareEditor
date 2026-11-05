# ADR 0014: Authoring scene hierarchy

Status: implemented locally, proposed for Joshua's review.

Store sparse child-to-parent links in the editor document; missing links mean root.
entity.reparent takes an entity and parent ID (0 for root), validates the candidate
forest and commits atomically. Reject missing references, cycles and depth above
64. Include links in existing snapshots/history so undo and failed batches restore
all relationships. Reparent has no spatial semantics until transforms exist.

scene.tree exposes deterministic parent-first flat nodes with parent/depth; sibling
order is ascending ID. editor.snapshot includes this tree, while legacy flat
scene.query stays compatible. The desktop indents this order, allows horizontal
scrolling and opens the same reparent command from context menus.

Duplicate remains a sibling of its source and copies no descendants. Deleting a
parent promotes direct children to scene root, preserving deeper relationships.
Document these semantics rather than implicitly deleting a subtree.

Read authoring JSON v1/v2. V1 has only root entities. V2 adds an explicit parent
field to every entity. Export v2 when links exist, otherwise retain v1; root-only
v2 input normalizes to v1. Validate complete forests before load/write. No config
schema change, dependency or engine integration is needed. Future engine parenting
and transform preservation requirements are recorded for Joshua in ENGINE_NEEDS.

Tests cover deterministic trees, cycle/missing/depth rejection, duplicate/delete
edges, atomic rollback/history and v1/v2 roundtrips. Windows, transforms, recursive
prefabs and runtime parenting remain future work.
