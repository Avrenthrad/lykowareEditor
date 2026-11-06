# Reusable standalone fixtures

All content here is original editor test data, not engine content. Test readers
must preserve these files; round-trip exports and generated workloads belong in a
fresh temporary directory. Fixed example project IDs are not creation defaults.

| Fixture | Purpose / expected outcome |
| --- | --- |
| valid/project/lykoware.toml | Project manifest v1; normalized identity/name/default directories |
| valid/project/scenes/main.json | Scene v3; two entities, root at x=3.25, child at y=2 |
| valid/project/prefabs/tree.json | Prefab v1 with scene v3; detached two-entity subtree |
| valid/project/assets.json | Catalogue v1; two valid sources, one unsupported script entry |
| valid/workspace-v2.toml | Workspace v2 with canonical starter docking layout |
| migration/scene-v1.json | Flat legacy scene; identities/names survive v3 promotion after transform edit |
| migration/scene-v2.json | Legacy hierarchy; parent links survive v3 promotion after transform edit |
| migration/workspace-v1.toml | Legacy settings migrate to v2 canonical docking without changing user settings |
| malformed/* | Truncation, scene cycle/zero scale, catalogue path collision, disconnected prefab, future project schema and duplicate workspace table; all rejected without changing editor state/history |
| generate_large.py | Generates a 1000-entity shallow scene and full 4096-entry mesh catalogue, deterministically, offline, without overwriting existing destination files |

The large catalogue is metadata only; it intentionally has no glTF resources.
Auditing it must report 4096 skipped kinds without trying to import/read meshes.
The small script entry is also unsupported metadata, intentionally without a file.
Mock output is simulated. These workloads prove repeatable correctness, not target
hardware performance or real runtime/renderer behavior.

Run all fixture checks through CTest's fixture_projects group, or from repository
root:

```bash
python3 editor/tests/fixture_project_tests.py editor/build/desktop/lyko
python3 editor/tests/fixtures/generate_large.py /tmp/new-lyko-fixture-folder
```

Fixture coverage includes every currently supported persisted format/version:
scene 1/2/3, workspace 1/2, prefab 1, catalogue 1 and project manifest 1. There are
no existing graph, imported-resource or build-profile formats to migrate yet.
Add fixtures whenever those formats or new migrations land. CLI/MCP parity and
no-state-change failure assertions are part of the fixture test group.
