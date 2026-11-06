# ADR 0028: Explicit persistent recent-project history

Status: implemented locally, proposed for Joshua's review. Checklist slice 9c.

Persist recent session metadata as versioned machine-local JSON history, separate
from portable project TOML configuration and scene/workspace/prefab/catalogue data.
The v1 record is {schema_version:1,recent:[{path,root,manifest}]}, preserving MRU order
and the last observed manifest metadata. This is session history data, not an
alternate project configuration format. It contains absolute local paths: choose
a user-local location and do not commit/share it as portable project data.

Expose project.recent.save(path) as an explicit no-overwrite filesystem export,
and project.recent.load(path) as an atomic undoable replacement of recents only.
Do not infer a home/config path, perform implicit startup reads or autosave settings.
Save/load between processes provides persistence; safe in-place replacement remains
step 12. Desktop Project-menu entries open the same named path forms.

Strictly validate store/version/entry/manifest fields, types, UTF-8, bounded absolute
normalized paths, containing-root relationship, manifest metadata, uniqueness and
16-entry limit. The entire replacement is validated again by the pure bus before
commit. Reject nonregular stores and reads over 1 MiB. Unknown/malformed/duplicate
entries fail as invalid_project_recents with a field; never partially import.

Loading stored recents does not open, stat or validate the stored project roots,
activate a project, change authored content or reorder entries. Missing/moved/stale
projects remain historical metadata. A later project.open rereads its current
manifest and applies normal content guards. Captured undo/redo never rereads the
store. Malformed imports preserve active context, authoring state and history.

There are no new dependencies or external service calls. Existing no-overwrite,
normal-filesystem race and power-loss limits apply. Windows, automatic startup
settings and broader dirty-aware project switching remain separate work.


Validation includes separate-process persistence, CLI/MCP parity, malformed/unknown/
duplicate/version/path rejection, no-overwrite exports, no implicit activation,
atomic grouped imports, captured redo after store deletion and native menu actions.
Generated recent-history fixtures cover the machine-local format without committing
real local paths. Actual Windows behavior remains unverified.
