# ADR 0018: Detached authoring prefabs

Status: implemented locally, proposed for Joshua's review.

Introduce prefab JSON v1 with type/schema_version/root/scene. Reuse validated
scene payloads (v1/v2/v3) for names, hierarchy and local transforms. Require one
rooted tree. Export only a selected subtree, remove its outside parent and
normalize file-local IDs to parent-first 1..N. Preserve local rather than world
placement. Emit one entity per line. These IDs are not durable override handles.

prefab.export publishes to a new file via existing bounded UTF-8/no-overwrite
publication helpers. It is an explicit external effect with no history mutation,
and is excluded from batches. Existing files remain untouched.

prefab.instantiate reads/validates the file into a typed authoring mutation,
allocates fresh IDs, remaps internal links and transforms, attaches the root to
the requested parent and selects it atomically. Revalidate typed source data,
allocation capacity and candidate hierarchy before commit. Use transaction-candidate
parent lookup so earlier batch rows can create that parent. Failed batches and
undo restore the entire previous scene/selection/allocation state. Redo uses the
captured document, not a reread of a potentially changed file.

Expose both registrations through existing CLI/MCP, palette and keymaps. Context
export prefills the clicked entity. No UI-only mutation or scene format change is
introduced. Update manual build commands for the new core source file.

Instances are detached copies. Linked definitions, override tracking, GUIDs,
recursive prefab references, runtime resources and engine integration are deferred.
The 1 MiB file and depth-64 hierarchy limits remain. Tests cover export isolation,
ID/transform remapping, selection/history, file preservation, malformed forests,
parent/exhaustion/depth rejection, batch rollback and CLI/MCP parity.
