# ADR 0016: Read-only world transform composition

Status: implemented locally, proposed for Joshua's review.

Define authoring coordinates as right-handed, Y-up, -Z-forward editor units.
Use column vectors, row-major serialized matrices, and local T*Rx*Ry*Rz*S for
intrinsic XYZ Euler degrees. World matrices are parent_world*local. Return the
complete affine matrix with explicit convention metadata from transform.world;
do not decompose it into TRS because nonuniform parent scale plus child rotation
can introduce shear. Retain signed-scale reflections.

Compute from a coherent service snapshot, walking ancestors iteratively. Reduce
angles modulo 360 before radians conversion and reject nonfinite multiplication
terms/sums with transform_range. Normal double rounding/underflow applies and
invertibility is not guaranteed. Invalid/missing targets produce structured errors.
Queries do not mutate scenes, revisions, history or persistence; derived matrices
are not saved. Inspector opens the same query through the existing form/result UI.

No world-space setter, inverse, decomposition or world-preserving reparent is
introduced. Reparent continues to preserve local values. The mock renderer remains
unchanged. Future engine conversion requirements are planning-only in ENGINE_NEEDS.

Analytic tests cover nested transforms, Euler order, shear, hierarchy undo,
reflection/large angles, overflow, unchanged history and CLI/MCP parity. Windows,
real rendering and numerical conditioning beyond finite arithmetic remain unverified.
