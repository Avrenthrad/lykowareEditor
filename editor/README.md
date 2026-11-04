# Standalone editor foundation

The editor currently runs headlessly with a deterministic mock backend. It has no
engine dependency, external libraries, display/GPU requirement or network listener.
The desktop shell and actual game runtime are not implemented.

## Build and test

Requires C++20, CMake 3.25+, Ninja and Python 3. From `editor/`:

```
cmake --preset dev
cmake --build --preset dev --parallel 2
ctest --preset dev
```

Use the `sanitizers` preset for ASan/UBSan. Configure with
`-DLYKO_CLANG_TIDY=ON` to run installed clang-tidy while compiling. On Windows use
a Visual Studio developer shell with Ninja and Python available. CMake, Windows
and clang-tidy checks have not run in the current cloud environment because the
tools/platform are unavailable; direct GCC builds below have been verified.

From the repository root, the verified fallback is:

```
mkdir -p /tmp/lyko-build
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Ieditor/src editor/src/command_bus.cpp editor/src/command_service.cpp editor/src/scene_io.cpp editor/src/mcp_server.cpp editor/src/main.cpp -o /tmp/lyko-build/lyko
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Ieditor/src editor/src/command_bus.cpp editor/tests/command_bus_tests.cpp -o /tmp/lyko-build/tests
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -Ieditor/src editor/tests/json_tests.cpp -o /tmp/lyko-build/json-tests
/tmp/lyko-build/tests
/tmp/lyko-build/json-tests
python3 editor/tests/cli_tests.py /tmp/lyko-build/lyko
python3 editor/tests/persistence_tests.py /tmp/lyko-build/lyko
python3 editor/tests/mcp_tests.py /tmp/lyko-build/lyko
python3 editor/tests/feedback_tests.py /tmp/lyko-build/lyko
```

Use the existing isolated checkout; do not create a worktree unless requested.
The GitHub workflow is manual-only and requests self-hosted runners with Linux
or Windows and X64 labels. It assumes these prerequisites are installed and runs
no artifact uploads. Joshua must arrange/authorize runners before dispatching it.
No hosted CI runs or paid services have been requested.

## Headless JSON CLI

Start `lyko --instance demo`, then send one JSON object per line:

```json
{"instance":"demo","command":"commands.list"}
{"instance":"demo","command":"entity.create","arguments":{"name":"Player"},"expected_revision":0}
{"instance":"demo","command":"play.start"}
{"instance":"demo","command":"play.input","arguments":{"value":"right"}}
{"instance":"demo","command":"logs.query"}
{"instance":"demo","command":"preview.capture"}
{"instance":"demo","command":"scene.query"}
{"instance":"demo","command":"history.undo"}
```

Responses contain `ok`, `revision` and either `data` or an `error` with `code`,
`field` and `message`. The CLI continues after invalid requests and exits 1 if
any failed. Messages and scene files are capped at 1 MiB, JSON nesting at 64.
IDs and revisions require exact unsigned integers; numbers are never coerced.
Requests reject unknown fields. Discovery returns command versions and input
schemas. Read [the command protocol](../docs/COMMAND_PROTOCOL.md) for transactions,
persistence and MCP integration.

History is in memory, using full before/after snapshots. It is not persisted and
has not been optimized for large projects. The mock records at most 1024 input
messages per session and retains the latest 256 log messages. A scene load resets
runtime state. Mock screenshots are deterministic 64x36 PPM images representing
state, not rendered gameplay. GUI frame-rate and handheld usability are untested.
