# ADR 0027: Explicit project directory creation

Status: implemented locally, proposed for Joshua's review. Checklist slice 9b.

Expose project.create(path, project_id, name, directories) as a named filesystem
export through the registry, CLI, MCP and Project menu. Path names a NEW project
root beneath an existing parent. A supplied identity remains unchanged 32-character lowercase hexadecimal data
(as in ADR 0025). Empty identity requests 16 bytes from std::random_device through
a uniform byte distribution, encoded as 32 lowercase hex characters. The desktop
form defaults to empty for this workflow. Generated identifiers are opaque, not
authentication tokens, signatures or a guarantee of global uniqueness. Directory overrides use the manifest model's defaults/validation.

Validate all configuration and the root basename before filesystem mutation.
Disallow directory roles occupying lykoware.toml or its descendants, ignoring
ASCII case. Resolve the existing parent and validate the captured root/manifest
session paths. Reserve the root with create_directory; existing roots, including
empty directories and symlinks, are never adopted, overwritten or cleaned up.
Create the configured directory tree, then publish lykoware.toml LAST through the
existing no-overwrite bounded text publisher. The manifest is the readiness marker;
the directory itself can be visible while setup runs. Do not promise atomic
whole-directory publication or power-loss durability.

On population/publication failure, attempt cleanup only after successfully
reserving that new root. Report project_creation_failed/path; if cleanup itself
fails, report that the destination requires inspection. Model/configuration errors
retain invalid_project_manifest fields. Normal filesystem checks do not create a
hostile-filesystem sandbox or protect against adversarial concurrent replacement
of the reserved root. Process crashes may leave an incomplete root without a
manifest; callers must inspect it rather than treating it as an openable project.

Creation leaves active context, authoring content, revision and history unchanged.
It is an explicit external export, not undoable, and cannot be a transaction row.
Return created=true and captured project {path, root, manifest}; do not open
implicitly. Desktop feedback offers the shared project.open form with the created
manifest prefilled. Opening retains all existing content guards and atomic history.
Undoing a later open never removes the created project from disk.

Tests cover configured/nested/Unicode folders, TOML/readback, instance/revision
preconditions, no-overwrite for file/directory/symlink destinations, namespace
conflicts, real post-reservation OS failure cleanup, concurrent-process reservation,
unchanged editor state, explicit activation, export metadata and CLI/MCP parity.
Native menu tests exercise Create navigation. Persistent recents remain slice 9c.
Windows, hardware and adversarial filesystem behavior remain unverified.
