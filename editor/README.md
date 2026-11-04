# Standalone editor foundation

No engine checkout, GPU, display, external libraries or network service is needed
for this foundation. Requires C++20, CMake 3.25+, Ninja and Python 3 for CLI tests.
From this directory:

```
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

The `sanitizers` preset adds ASan and UBSan on GCC/Clang. Run clang-tidy against
`build/dev/compile_commands.json`. Windows uses the same dev preset from a Visual
Studio developer shell with Ninja available. These CMake paths await validation
in this environment; CMake and Ninja are currently unavailable.

A verified fallback from the repository root, with outputs outside the checkout:

```
mkdir -p /tmp/lyko-build
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Ieditor/src editor/src/command_bus.cpp editor/src/main.cpp -o /tmp/lyko-build/lyko
g++ -std=c++20 -Wall -Wextra -Wpedantic -Werror -pthread -Ieditor/src editor/src/command_bus.cpp editor/tests/command_bus_tests.cpp -o /tmp/lyko-build/tests
/tmp/lyko-build/tests
python3 editor/tests/cli_tests.py /tmp/lyko-build/lyko
```

Use the existing isolated checkout, without a new worktree. Start a headless
session using `lyko --instance my-session`, then send commands on stdin:

```
commands.list
entity.create "Player"
play.start
play.input "jump"
scene.query
history.undo
scene.query
```

This initial CLI uses one command per line and quoted string arguments. Results
are text with stable error codes; JSON requests/results and MCP are not yet
implemented. The process exits 1 if any line failed and continues processing so
agents can query state after errors. The C++ bus supports grouped transactions
and optional expected revisions; CLI batch/revision syntax is pending.

The mock records input and play state only. No real simulation, screenshots,
renderer, asset persistence or desktop UI is provided yet. All history is in
memory and lost on exit. Full before/after snapshots prioritize correctness;
large-scene history compaction is future work. EngineBridge implementations must
be pure value transformations to preserve rollback guarantees.
