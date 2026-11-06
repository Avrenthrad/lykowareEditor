# ADR 0025: Standalone project manifest v1

Status: implemented locally; proposed for Joshua's review.

Define an editor-owned ProjectManifest value and a small, documented TOML v1
schema. The conventional file is lykoware.toml. Required root keys are
schema_version = 1, project_id (32 lowercase hexadecimal characters supplied by
the caller), and name (1–128 UTF-8 bytes, no ASCII controls/DEL). Identity is
opaque, preserved on load/save and unrelated to engine IDs. Duplicate project IDs
across copied projects are not detected by this format; creation/clone policy is
part of the later lifecycle work.

An optional [directories] table supplies assets, scenes, scripts, graphs and cache;
omitted fields default to assets, scenes, scripts, graphs and .lyko/cache. Paths
use the existing portable project-relative asset-path validation. Source folders
may nest (e.g. assets/scenes); the cache may not equal, contain or descend from any
source directory, comparing ASCII case-insensitively. Paths are metadata: parsing
does not create directories, inspect resources or follow symlinks. Lifecycle will
bind the root to the manifest's containing directory, not serialize an absolute
machine path. This slice does not activate a project or change scene/history.

Reject unknown/duplicate fields/tables, unsupported schema versions and malformed
values before publishing. Support a deliberately narrow TOML syntax: bare schema
keys, one [directories] table, decimal version 1, single-line basic/literal strings,
comments and CRLF. Basic strings support standard escapes including scalar \u/\U;
surrogate escapes are rejected. Multiline strings, dotted/quoted keys, inline
tables, numeric extensions and other TOML constructs are not accepted. The writer
emits deterministic full TOML in this subset. This is not a general TOML parser;
reuse existing bounded text publication and UTF-8 helpers without a new dependency.
A future general parser can replace the syntax adapter while retaining this model.

Expose project.manifest.validate(path) and project.manifest.write(path, project_id,
name, directories) through the shared registry. The directories object supplies
optional role overrides; omitted roles default as above. All four write arguments
are required by the registry; use directories = {} for defaults. Validation returns normalized manifest metadata without
state changes. Writing validates the whole value, then publishes a NEW file only;
existing destinations are preserved. It is an explicit filesystem export, cannot
be a transaction mutation and is not undone. Errors are structured as
invalid_project_manifest with the offending field or path. Target/revision/schema
preconditions precede file access. Reads reject nonregular files before opening;
publication retains existing hard-link/filesystem and power-loss limitations.
The 1 MiB text limit applies. No implicit directory creation or scene/catalogue
persistence changes are introduced.

Contract tests must check Python tomllib compatibility of generated TOML, Unicode
and escaping, default/partial/full directories, ordering, validation and path/cache
rules, malformed/duplicate/unknown input, unsupported versions, bounds, existing
file preservation, no state/history changes and CLI/MCP parity. Windows remains
unverified until explicitly tested. Project lifecycle is checklist step 9.
