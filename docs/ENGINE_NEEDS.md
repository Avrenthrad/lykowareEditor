# Future engine needs

Planning only. Joshua decides whether and how integration happens. Do not access
or depend on another repository to satisfy this list.

- A versioned capability handshake and explicit editor instance/session identity.
- Scene snapshots with stable identifiers and structured validation errors.
- Atomic mutation batches with revision checks and restorable state for undo.
- Play/stop and deterministic input injection with observable completion.
- Bounded structured logs, live scene queries and asynchronous screenshots.
- Resource import, preview and cooking interfaces with cancellation and progress.
- Renderer surface ownership, resize and lifetime contracts, decided separately.

Until integration is approved, implement and test these contracts with the mock.

- Future integration must define parent/root semantics, cycle/depth validation,
  and transform preservation when reparenting; current hierarchy is authoring-only.

- Local transform interchange needs agreed units/basis, Euler conversion, world
  composition and reparent world-preservation semantics. Current values are
  authoring-only and must not be assumed to drive the mock runtime/preview.

- Authoring world matrices now use right-handed Y-up/-Z-forward editor units,
  column vectors, row-major storage and T*Rx*Ry*Rz*S local order. A future bridge
  must explicitly match/convert these conventions; no engine assumptions were read.
