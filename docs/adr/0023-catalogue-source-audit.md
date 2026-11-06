# ADR 0023: Read-only catalogue source audits

Status: implemented locally, proposed for Joshua's review.

Expose asset.audit(root) through the shared registry, CLI, MCP and desktop worker
queue. Capture the committed catalogue and inspect entries in ascending numeric
ID order using the existing scene/prefab validators. Validate the absolute project
directory even for an empty or unsupported-only catalogue. Invalid roots fail the
command; individual source failures produce structured per-entry diagnostics and
do not prevent later entries from being checked. Unsupported kinds are explicitly
skipped without reading their files. No new importer or engine dependency is added.

Reports contain total/valid/invalid/skipped counts and one entry for every captured
catalogue record. Valid entries retain exact-byte SHA-256, size and entity count;
invalid entries retain code/field/message. Successful command completion means the
report was produced, not that every source passed. Entry order is deterministic;
filesystem-dependent diagnostics and source contents can change between calls.

Audits do not mutate state, revision, history or files, and cannot be transaction
mutations. They run sequentially with the existing 4096-entry catalogue and 1 MiB
per-source limits. Memory retains the report and one parsed source at a time, not
all source documents. Desktop remains asynchronous, but an audit occupies the
serialized command worker until completion and has no cancellation/progress API.

The Assets browser opens the common root form. Feedback shows counts and its usual
bounded detail preview; CLI/MCP return the full report. There is no persistent
baseline, scanner, dependency graph or linked-prefab behavior. Ordinary canonical
path checks remain best-effort and do not provide a hostile-filesystem sandbox.
A report observes files sequentially and is not an atomic filesystem snapshot.

Tests cover mixed supported/unsupported content, malformed/schema/oversized/missing
sources, root failures including empty catalogues, deterministic IDs, exact hashes,
contained path policy, FIFO rejection where available, unchanged history/state,
precondition rejection and CLI/MCP parity. Windows and hardware remain unverified.
