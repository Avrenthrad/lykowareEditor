# ADR 0004: Desktop execution queue and coherent panel snapshots

Status: Implemented locally; review pending

## Context

Commands can load files or wait on a backend. Calling the synchronous command
service from a render/input thread would freeze the desktop. Independent per-panel
queries can also mix revisions when another agent edits the same instance.

## Decision

Add a toolkit-independent CommandQueue under `editor/src/ui/`. Its single worker
calls the existing CommandService; the queue has no mutation handlers or alternate
state. Submission and polling use try-locks and do not wait for backend execution.
Transport serialization still happens on the caller; this is not a real-time or
60fps guarantee. Keep requests small and reuse immutable query results per frame.

Assign accepted requests FIFO tickets. Capacity defaults to 64 (configurable
1–256) and counts queued, in-flight and completed-but-unpolled work. Request JSON
is capped at 1 MiB. A busy/full queue rejects submission explicitly without
assigning a ticket; the caller retains its request and may retry on a later frame.
Accepted edits are never replayed automatically. Snapshot output and in-memory
history can still grow; count bounds are not a total byte-memory guarantee.

Close rejects new requests and cancels requests not yet dequeued. A dequeued call
is in flight and may commit normally. The worker does not interrupt filesystem or
backend operations. Destruction requests shutdown and joins the worker, so it is
an application-shutdown operation, never a per-frame action. The service and its
backend must outlive the queue. Cancelled/transport-failure responses use revision
zero as unknown; they must not reset a frontend's cached revision.

Expose `editor.snapshot` through the registry for CLI, scripting, MCP and desktop.
It copies scene, workspace, inspector selection, tokens, logs and history from one
locked revision. It does no file I/O and no software image rendering. Fetch preview
images separately with the snapshot's expected_revision; discard stale frames.
All rendering remains on the eventual desktop thread; workers publish data only.

Expose `history.query` with the transaction cursor and immutable original commit
revision, command names, mutation count and applied/redo status for each entry.
A batch has one record. Failed/no-op edits add none. New edits after undo replace
the redo branch. The named service passes registry command names into the bus;
low-level C++ transactions without metadata use the generic name `mutation`.

## Validation and limits

Tests stall a backend using semaphores, then exercise submission and polling,
capacity, FIFO results, instance errors and shutdown cancellation. Snapshot tests
cover consistent panel data, named batch history, undo/redo, no-op behavior and
branch replacement. This tests the adapter contract, not actual desktop input,
window rendering, frame latency or external engine behavior.
