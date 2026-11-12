# ADR 0008: Typed transaction drafts

Status: implemented locally, proposed for Joshua's review.

## Decision

Provide a typed row builder when the palette opens transaction.execute. Discover
choices from the registry's mutation flag, excluding transaction.execute because
the service disallows recursive batches. Do not duplicate a list of command names
in the UI. Render each row with the existing standalone command form. Users may
add, remove and reorder rows before one explicit atomic submission.

Validate the complete draft against discovery without executing any child. Show
field errors inline and numbered batch errors; disable submission while invalid.
Limit GUI drafts to 64 rows. Keep protocol bounds unchanged. Stable widget IDs
follow rows during reordering. Defer structure edits until drawing is complete;
do not submit a stale order in the frame where rows change.

Send the resulting commands array through the same revision-guarded desktop
queue and service used by ordinary commands, CLI and MCP. Domain checks remain
on the bus; a later-row failure rolls back the entire batch. Back/search navigation
discards transient drafts. Layout objects retain advanced JSON; recursive nested
transaction execution is not introduced.

## Validation and limits

Model tests cover empty/oversized batches, excluded commands, invalid/missing
fields, row order, no draft side effects, atomic undo/redo and failed-batch state
and history preservation. SDL input opens the palette builder, adds two rows,
submits and checks atomic undo/redo. Windows and external accessibility testing
remain outstanding. Existing transaction protocol and persistence are unchanged.
