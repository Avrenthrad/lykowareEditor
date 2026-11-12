# ADR 0020: Read-only registered source validation

Status: implemented locally, proposed for Joshua's review.

Add asset.validate(id, root) as a query through the existing command service.
Require an explicit absolute existing project directory per call; do not introduce
implicit cwd-dependent resolution or persist a machine-specific root in the catalogue.
Resolve the relative registered path canonically, compare path components for
containment, require a regular file, then use existing bounded reads and scene/
prefab parsers. Return logical identity, kind, byte/entity counts or structured
root/path errors. Unsupported kinds remain explicitly unsupported and are not read.

Keep all work on the existing desktop command worker. The Assets browser opens
the shared form with ID prefilled and disables unsupported kind actions. No import,
cache, source mutation, history entry or persistence schema change is introduced.

Canonical checks are observations followed by a separate open/read. They are not
race-proof filesystem containment or a hostile-filesystem sandbox. Contained
symlinks are allowed; observed escapes fail. Results validate bytes read rather
than guarantee future path identity. Rooted platform handle APIs and fingerprints
must be designed before stronger guarantees or side-effecting import are claimed.

Tests cover CLI/MCP parity, unchanged state/history, Unicode paths, missing/relative/
non-directory roots, missing/nonregular/oversized/invalid sources and symlink escape
rejection where supported. Windows remains unverified; kinds other than scene and
prefab, source tracking and actual import remain future work.
