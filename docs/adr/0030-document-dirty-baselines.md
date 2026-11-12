# ADR 0030: Persisted document observations and dirty state

Status: implemented locally, proposed for Joshua's review. Checklist step 11.

Track canonical persisted values for scene, catalogue, workspace, active project
settings and recent history. A document is dirty when its current persisted value
differs from the last successful load/save observation; an untitled default is
clean but unsaved. Runtime, selection and other transient state do not enter scene
or workspace clean comparisons. Display availability, dirty, unsaved and last
observed file path through document.status and coherent editor.snapshot metadata.

Persistence observations belong to the serialized command service, not undoable
authoring snapshots: undoing an edit can return to a saved baseline; undoing a save
cannot change the disk. Capture load baselines from decoded typed values, apply
only after the entire transaction succeeds, and retain loaded baselines when later
rows edit those values. Failed decode/commit/save never changes observations.
Paths describe the last successful observation, not a filesystem watch or an
in-place overwrite capability. Exact-byte fingerprints for existing-file writes
are a separate next step.

Project baselines are keyed by manifest path so context undo can compare each
project to its own observed values. project.configure changes only name/directory
metadata, preserving identity and root; files never move. project.settings.revert
restores last observed settings as an undoable mutation. Context transitions guard
against unsaved project settings, including staged configure/close: bus state keeps
its captured open baseline, while service-provided clean origins account for later
successful external saves. These guards complement conservative authoring-content
protection and do not implement dirty-aware document switching yet.

Show a modified-document count and Project-menu status/settings/revert forms over
the same registry. Authoring file schemas remain unchanged; observations and open
baselines never enter project/scene/prefab/catalogue/workspace exports. Save/export
success changes observation metadata without advancing the authoring revision.
Queries and failed operations preserve both. Selection/runtime commands can still
advance authoring history without dirtying scene contents.

Tests cover clean/unsaved defaults, independent persisted kinds, successful/failed
exports, undo/redo against non-undoable disk observations, grouped load/edit and
failed-batch behavior, project settings/revert guards and CLI/MCP parity. Windows,
external file watching and hardware performance remain separate validation work.
