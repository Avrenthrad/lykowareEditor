# ADR 0015: Local authoring transforms

Status: implemented locally, proposed for Joshua's review.

Store sparse nonidentity local transforms: position XYZ, intrinsic XYZ Euler
rotation in degrees, signed scale XYZ. Require finite doubles and nonzero scale;
negative scale supports reflection. Defaults are zero position/rotation and unit
scale. JSON numeric components use IEEE-754 double precision; IDs stay exact uint64.

Expose transform.set/query through the shared registry. Sets replace all nine
components atomically with revision guards and undo/redo. Generated forms gain
finite signed/fractional numeric controls. Inspector opens the same form with
selected-entity values. Duplicate copies the transform; Delete removes it.
Reparent and parent removal keep local values. World composition/preservation,
coordinate basis/units integration and runtime interpretation are deferred and
recorded in ENGINE_NEEDS. The mock preview does not render these values.

Write scene JSON v3 when nonidentity values exist, with parent and transform fields
on every entity. V1/v2 load with identity values; identity-only export follows the
previous v1/v2 policy. Validate before load/write; failed batches restore all data.
No engine dependency, new library or config format change is introduced.

Tests cover numeric draft validation, command errors, history, duplication/deletion,
batch rollback, v3 roundtrip, invalid loads, legacy identity defaults and CLI/MCP
parity. Windows, world transforms and real spatial rendering remain unverified.
