# ADR 0002: Headless protocol and scene publication

Status: Implemented locally; review pending

## Context

The initial standalone foundation must be testable without network access or GUI
libraries. ADR 0001 establishes a common command bus and diffable authoring data.

## Decision

Expose JSON-lines CLI and stdio MCP through a single serialized CommandService.
Its registry supplies versioned names, argument schemas, descriptions and typed
mutation decoders. Transport adapters only translate envelopes. Keep exact
unsigned 64-bit identifiers and revisions; reject numeric coercions. Bound input
messages and scene files to 1 MiB and JSON nesting to 64 for the initial version.

Use editor-owned JSON parsing/serialization for the current protocol, with no
third-party code. Cover strict syntax, Unicode, duplicate keys, integer boundaries,
number handling and round trips with tests. Reconsider a pinned permissive JSON
library when dependency acquisition and review are available; changing the parser
must preserve public protocol behavior.

Scene schema v1 stores entities and next-ID metadata. It excludes play state,
input and logs. Load prepares and validates a complete replacement before entering
an undoable transaction. Save is explicitly an external export, separate from
undo history. Publish a complete sibling staging file through a no-overwrite hard
link. Preserve existing files, and fail safely on unsupported filesystems rather
than copying a partially visible destination. In-place replacement and crash
recovery need a later cross-platform design; this is not a durability guarantee.

Mock capture returns a deterministic PPM software image, labelled simulated.
Bound input history to 1024 entries and logs to the latest 256 messages. All mock
runtime changes and their logs are restored by undo/redo. This proves the agent
feedback plumbing, not real gameplay, rendering or performance.

CI is manual-only and uses existing self-hosted Linux/Windows runners to avoid
requesting paid hosted execution. No artifacts are uploaded. Automatic triggers
and hosted capacity require Joshua's authorization and verified runner budgets.

## Consequences

Agents can create a scene, simulate input, inspect logs/images and undo changes
without windows or engine access. Custom protocol code and hard-link portability
need explicit maintenance/testing. External MCP interoperability, CMake execution,
Windows checks and clang-tidy remain unverified in the current cloud environment.
