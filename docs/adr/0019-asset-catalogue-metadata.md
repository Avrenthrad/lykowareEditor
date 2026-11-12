# ADR 0019: Asset catalogue metadata

Status: implemented locally, proposed for Joshua's review.

Store an independent catalogue in the editor document: monotonic uint64 IDs,
relative metadata paths and declared kinds. Register/relocate/forget use typed
mutations within the existing atomic candidate/history model. Relocation preserves
IDs and removal never rewinds allocation. No source filesystem effects occur.
Paths follow bounded lexical portable-style rules and are unique under ASCII case
folding. Unicode equivalence, project-root/symlink resolution and file decoding
are deferred; this is not a filesystem sandbox or verified import pipeline.

Persist separately as strict JSON v1 with next_id and one asset per line. Loading
replaces only catalogue state undoably; saving uses the existing bounded atomic
no-overwrite helper as an explicit external effect. Keep scene, prefab and workspace
formats unchanged. IDs persist within this catalogue; do not claim global GUIDs.
Limit entries to 4096 and paths to 512 UTF-8 bytes. The 1 MiB file bound remains,
so not every maximum-size in-memory catalogue is exportable without shortening
paths. Preserve state and existing files when validation/publication fails.

Expose all operations through registry discovery, CLI/MCP and normal forms. The
modeless Assets browser consumes coherent snapshot metadata, filters with the
same ASCII folding semantics as asset.list, clips offscreen rows, and opens
prefilled shared forms. Search/navigation is transient UI state. Do not scan,
copy, move or delete source files, even for Relocate/Forget actions.

Tests cover identity/history, path rules/duplicates, rollback, manifest validation,
scene isolation, CLI/MCP parity and native browser forget/undo. Importers, hashes,
dependencies, watching, global identity and engine resources remain future work.
No library/dependency, engine access or public publication is required.
