# ADR 0001: Standalone command-driven editor

Status: Proposed for Joshua's review

## Context

Lykoware Editor serves humans and autonomous agents through the same capabilities.
It must develop, run and test independently of any engine repository. Engine
integration requires a later decision by Joshua.

## Decision

Use C++20 and CMake presets. Keep implementation under `editor/`, external
libraries under `third_party/`, and architecture and handover documents in `docs/`.

All frontends submit named commands to one command bus. Each registration owns
its versioned input/output schema, documentation, validation and handler. CLI,
GUI, scripting and MCP expose this registry without independent mutation paths.
Queries do not create history. Mutations validate completely before committing an
atomic transaction; failures leave state and history unchanged. Undo and redo
restore document and mock runtime state from recorded before/after values. A new
mutation after undo discards the redo branch. Grouped operations commit once.
External effects such as export are explicit operations: undo restores editor
state but cannot retract files already consumed by another process. Such effects
must be documented and must never silently overwrite unrelated files.

The editor owns stable entity identifiers, scenes, prefabs, assets and procedural
graphs. Persist machine data as versioned JSON, configuration as TOML and meshes
as glTF. Graph nodes and connections have a text representation editable through
the bus. Load validates an entire candidate document before replacing live state.
Save uses a temporary sibling file and atomic replacement where supported.
Binary cooking is a separate shipping operation, never the authoring source.

Define an abstract `EngineBridge` under `editor/src/backend/`. It exchanges editor
DTOs and structured results, with no external engine headers, libraries or copied
code. The default mock implements deterministic play/stop, simulated input,
scene queries, logs and software screenshot generation. Mock output is explicitly
identified as simulated; it does not establish real engine correctness.

Select Dear ImGui's docking branch (MIT) for the initial desktop shell, with SDL3
(zlib) for platform/window/input integration. Before use, vendor exact reviewed
commits, licence texts and version records, behind editor-owned wrappers. No
dependencies are vendored by this ADR. Headless builds must not require either
library, a display server or a GPU. Rendering initially uses a software preview;
no engine or Vulkan integration is implied.

Central design tokens define colour, type, spacing, scaling and motion. Use one
permissively licensed icon family, pinned and recorded before inclusion. The
command palette searches the entire registry; context menus derive shortcuts
from a rebindable keymap. Persist dock layouts, scaling and accessibility themes.
Keep long operations on workers, publishing validated results on the editor
thread. Measure frame latency on target hardware before claiming 60fps readiness.

The CLI supports structured JSON requests/results and headless operation. MCP
uses stdio JSON-RPC with protocol negotiation, capability discovery and tools
derived from the command registry. Every tool call includes an explicit editor
instance identifier; missing or mismatched targets fail before execution. Do not
open a network listener by default. Serialize mutations per instance; concurrent
clients receive document revisions and stale-revision errors.

## Consequences and validation

The mock permits deterministic CI without engine access. GUI polish and real
runtime behavior remain separate milestones. Tests must cover validation without
mutation, transaction rollback, undo/redo branching, persistence round trips and
invalid documents, frontend parity, instance isolation and the mock feedback loop.
Linux and Windows CI build and test the standalone editor. Linux sanitizer and
clang-tidy jobs are required; artifact uploads remain opt-in and size-limited.

## Alternatives

A custom UI adds text, accessibility, input and docking maintenance before core
workflows are proven. Qt introduces licensing/distribution choices outside the
requested permissive-only policy. Dear ImGui is practical for the first shell,
but accessibility and touch/controller navigation require explicit validation.
