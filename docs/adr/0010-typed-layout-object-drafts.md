# ADR 0010: Typed layout object drafts

Status: implemented locally, proposed for Joshua's review.

## Decision

Provide a specialized nested object widget for layout.replace, currently the only
object-valued command field. Expose split direction/share, active tabs, ordered
tabs, panel placement/hiding and split creation. Populate panel choices from the
published layout schema. Label groups by their content; structural node IDs remain
internal widget identifiers. Expand the root and disclose deeper controls on demand.
Do not claim a generic recursive JSON Schema renderer.

A toolkit-independent LayoutForm owns a validated private DockLayout. Each edit
copies, changes, validates and canonicalizes a candidate before replacing the
draft. Failed edits preserve the valid tree. Apply structural edits after drawing
to avoid invalidating traversed references. Keep tree widget IDs independent of
changing display labels. Display ratios as percentages with 0.01% precision,
converting edited values to integer basis points; untouched values remain exact.

Standalone forms prefill the confirmed snapshot. New batch rows start with a
valid starter layout and use the same widget. Submission produces the existing
layout.replace object through the same revision-guarded queue and service. Draft
editing creates no live mutation or history. A committed layout remains one
atomic undoable operation, including within a larger transaction.

Retain a bounded advanced JSON import. Text changes block execution and typed
editing until explicitly imported or discarded. Parse/graph errors preserve the
last valid draft. Invalid typed candidates also block execution until corrected
or explicitly acknowledged. This avoids silently submitting stale valid data
while the user is looking at an invalid attempted edit.

## Validation and limits

Model tests cover rejected edits, structural operations, exact ratios, tab state,
canonicalization, all-hidden restoration, JSON roundtrip, no draft side effects
and one-step bus undo/redo. SDL interaction edits a split share, checks no live
change before Execute and exactly one committed revision, exercises undo/redo,
and checks malformed JSON import/discard gating. Windows, target hardware and
full accessibility remain unverified. No protocol, dependency, engine integration
or persistence change is needed.
