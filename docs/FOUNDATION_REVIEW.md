# Foundation review notes

Baseline reviewed: 1b38fb262a0f45b347534bd4dfcf588b513eab56.
Main refreshed: 8308edf552608cbd583747cbfd20f0bdd2d67141. No upstream changes.
Scope: initial targeted correctness pass on command service preconditions/effect
metadata, direct bus entity mutations, candidate/history commit behavior, async
queue execution, bounded text publication and selected asset/prefab contracts.
This is not a completed review of every stacked branch or a security audit.

## R001: Direct-bus entity names could violate persisted UTF-8 contracts

Create/rename/duplicate and ReplaceScene checked lengths, but accepted malformed
UTF-8 when called directly from C++. JSON transport validation did not protect
these entry points. A successful mutation could leave names that JSON serialization
rejects, violating the validate-before-commit contract.

Fix: reject malformed UTF-8 before committing these operations, matching the
existing prefab validator. Regression tests call the typed bus directly and check
create rejection, rename/duplicate batch rollback, unchanged history/revision and
invalid scene replacement. Targeted command_bus, headless_cli, scene_persistence
and prefabs tests pass in desktop and ASan/UBSan builds (local leak detection off).
Clang-tidy compilation reports no project findings.

## Outstanding

- Review all remaining ADRs/branch deltas and record findings/fixes against commits.
- Joshua's review and merge decisions remain outstanding; nothing is merged here.
- Windows/target hardware, actual MCP-client interoperability and approved CI runs
  remain checklist work. Existing automated tests do not establish these outcomes.
- Later standalone slices continue privately at the user's request; they do not
  assert foundation approval or grant access to an engine repository.
