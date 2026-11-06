# ADR 0022: Source byte fingerprints and guarded consumption

Status: implemented locally, proposed for Joshua's review.

Use SHA-256 of exact bounded source bytes, computed from the same in-memory text
used for JSON parsing. Do not hash reserialized JSON or reread files for hashing.
Vendor unmodified MIT PicoSHA2 v1.0.1 at commit
161cb3fc4170fa7a3eca9e582cebd27cc4d1fe29. Verify the header checksum in an offline
CMake wrapper and expose only content_hash string APIs, not vendor types. Retain
its licence and provenance in third_party; future distributions must include notices.

Add digest metadata to asset.validate, read-only asset.check for explicit token
comparison, and asset.instantiate_checked for guarded prefab consumption. Tokens
are strictly 64 lowercase hexadecimal characters. Valid changed sources yield
matches=false in checks; checked mutations fail as asset_source_changed before
allocation. Invalid sources remain errors. JSON parsing can fail before comparison.
Retain catalogue-candidate guards, staged-parent support and redo from captured
state. Existing unchecked commands keep their semantics.

Expose selectable tokens and prefilled Recheck/Instantiate-validated-bytes forms
in result feedback. Navigation is transient; every source operation still uses the
registry/worker queue. No catalogue baseline, persistence change, watcher or linked
prefab state is introduced. Digests are not signatures, path identity, file locks
or an improvement to the documented filesystem race guarantees.

Tests use standard empty/abc/multiblock/million-byte vectors, all byte values and
Python hashlib at SHA padding boundaries plus the source size limit. Command tests
cover exact-byte changes, comparisons, invalid tokens, checked instantiation,
rollback and CLI/MCP parity. Runtime import, persistent tracking, Windows and
adversarial concurrent filesystem behavior remain future work/unverified.
