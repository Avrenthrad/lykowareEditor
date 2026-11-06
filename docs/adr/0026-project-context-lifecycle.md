# ADR 0026: Undoable project context and session-local recents

Status: implemented locally, proposed for Joshua's review. Partial checklist step 9.

Add ProjectState to the editor document: optional active ProjectSession and up to
16 recent sessions in MRU order. A session captures validated manifest metadata,
canonical absolute manifest path and its containing root as generic UTF-8. Paths
are bounded to 32768 bytes each in the value model, not a promise about OS limits.
The pure project model owns manifest/path validation; the IO adapter resolves and
reads sources before producing a captured OpenProject mutation. The bus performs
no filesystem IO and revalidates typed inputs before commit.

Expose project.open(path), project.close, project.query and project.recent.clear.
Open/close/clear are ordinary named atomic undoable mutations and may participate
in transactions. Target/revision/schema checks precede source reads. Queries and
snapshot project fields return coherent active/recent metadata without reading
files. Recent-path navigation uses project.open again, so missing/invalid/changed
manifests are checked rather than trusting old metadata. Exact current-context
reopen is a no-op and does not repopulate cleared recents. Other opens deduplicate
by canonical path, insert at the front and truncate to 16; copied project IDs at
different paths are not treated as the same project.

Context operations never load, clear, save or reassign scene/catalogue contents.
Until dirty/document ownership work is implemented, changing active context or
closing it requires the scene and catalogue to be fully reset (empty entries and
next IDs 1), and the mock runtime to have no play/input/log state. Reject otherwise
as project_has_content/project. Deleting every entity alone is not a reset, and
saving a nonempty scene does not bypass the guard. Explicit scene/catalogue load
commands can reset those documents before a context change, including within one
intentional transaction. A staged edit before opening causes complete rollback;
opening then creating content is one valid transaction. This is conservative
content protection, not general dirty-state tracking (step 11).

Undo/redo restores context, MRU metadata and any grouped authoring state from the
same history snapshot. Redo does not reread deleted/changed manifests. It restores
a past observation rather than proving that a folder still exists. Future source
operations must revalidate paths/content independently. Canonical path checks are
ordinary best-effort filesystem operations, not a hostile-filesystem sandbox.

A Project menu opens the common write-manifest/open/close/query/clear forms and
recent-path forms. All execution remains on the shared command worker. Main-menu
status reflects whether a project is open. Scene/prefab/catalogue/workspace formats
are unchanged and exclude project paths/recents. No dependency or engine access.

Recents are session-local and undoable, not saved to user settings yet. Creating a
complete project directory, identity generation, persistent recents, dirty-aware
switching/recovery and active-project asset root resolution remain later slices.
Writing a new manifest remains the explicit non-undoable no-overwrite export from
ADR 0025, followed by project.open. This does not finish checklist step 9.

Tests cover canonical/alias paths, MRU order/bounds, input/candidate revalidation,
content guards, no-op opens, batch rollback, undo/redo after source deletion,
context-free persistence, CLI/MCP parity and native Project-menu navigation.
Windows, target hardware and hostile concurrent filesystem behavior are unverified.
