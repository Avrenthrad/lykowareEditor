# ADR 0007: Transaction-backed dockspace layouts

Status: implemented locally, proposed for Joshua's review.

## Context

The starter slot model cannot describe native nested splits, tab order or active
tabs. GUI-only persistence would break command parity and undo/redo.

## Decision

Represent layouts as a bounded split/tab tree plus hidden panel IDs. Require
complete, exactly-once panel coverage and reject malformed graphs before writing.
Canonicalize structural node IDs and prune empty regions. Ratios use basis points;
node IDs are paths rather than stable external handles. The shared command service
exposes layout.query, layout.replace and layout.reset. Replacement is atomic and
undoable, including when batched with scene edits.

The desktop adapter translates confirmed trees to ImGui and captures completed
native gestures. It submits one replacement with the revision at gesture start.
Queue rejection, stale revisions and invalid proposals restore confirmed state.
Transient mouse movement creates no history entries. Native pixel rounding is
compared against a settled baseline; window resizing reapplies relative layout.
Floating windows are unsupported and restored after an inline error.

Workspace TOML writes v2 with layout/node tables and reads v1/v2. Version 1
constructs the original starter layout. Legacy panel.dock/layout.resize operations
explicitly return to starter presets; layout.query is authoritative for custom
layouts. ImGui ini persistence stays disabled.

## Consequences and validation

The model remains toolkit independent and CLI/MCP editable without a window.
Nested palette objects currently use an advanced JSON editor. Unit/black-box tests
cover validation, rollback, history, migration and independent TOML parsing.
Synthetic SDL interactions cover resize, merge, reorder, active tabs, splits and
undo/redo, plus unsupported floating-drop rollback. X11/Xvfb capture was inspected.
Windows, target hardware performance and full accessibility remain unverified.
Frame pacing uses the remaining budget, without a hardware frame-rate guarantee.
