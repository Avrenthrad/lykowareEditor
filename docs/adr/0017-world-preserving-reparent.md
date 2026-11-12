# ADR 0017: Conservative world-preserving reparent

Status: implemented locally, proposed for Joshua's review.

Add a separate entity.reparent_world command so the existing local-preserving
operation stays compatible. Validate the candidate forest, compute the inverse
new-parent world matrix times old world, and decompose the required local matrix
into the existing T*Rx*Ry*Rz*S representation. Represent reflection on X scale and
choose a valid Euler solution, including gimbal cases; numerical values need not
match a previous equivalent decomposition.

Reject shear, nonfinite arithmetic, elimination pivots or local scale magnitudes
below 1e-12. Require parent-inverse residual, local recomposition and every affected
subtree world matrix to pass component-wise 1e-9*max(1,abs(original)) tolerance.
Reject unsafe cases as cannot_preserve_world/parent. Do not silently project shear
away or add a matrix override. This conservative subset is deliberate; it does not
promise all finite TRS cases succeed. Same-parent operations are no-ops.

Perform the work inside the bus candidate transaction so preceding batch mutations
are respected and all failures roll back. Undo restores both links and local values.
Use the already validated forest for bounded per-ancestor world calculations,
avoiding repeated whole-forest validation per descendant. Run through the existing
worker queue in the desktop. No persistence/config/dependency/engine change is needed.

Tests cover reflected transforms, Euler/gimbal cases, subtree stability, detach,
undo/redo, same-parent no-op, shear/unsafe-scale rejection, cycle/missing-parent
errors, batch rollback and CLI/MCP parity. World preservation is finite-precision;
Windows, real rendering and arbitrary ill-conditioned matrices remain unverified.
