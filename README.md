# Lykoware Editor

**Project checklist:** [Remaining steps and progress](docs/DELIVERY_CHECKLIST.md)

Standalone, command-driven editor for Lykoleon Studios. The current implementation
provides a standalone ImGui/SDL3 desktop, headless JSON CLI, shared-instance stdio
MCP, transactional editing and a deterministic mock backend. Final UI polish and
a real game runtime are future work.

- [Build, test and run](editor/README.md)
- [Command protocol](docs/COMMAND_PROTOCOL.md)
- [Status and limitations](docs/STATUS.md)
- [Private review stack](docs/REVIEW_STACK.md)
- [Work board](docs/WORK_BOARD.md)
- [Roadmap and milestone acceptance](docs/ROADMAP.md)
- [Architecture](docs/adr/0001-editor-architecture.md)

No engine repository, headers or libraries are required or integrated.
