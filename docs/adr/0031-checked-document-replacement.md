# ADR 0031: Checked existing-file saves and exact observations

Status: implemented locally, proposed for Joshua's review. Checklist step 12.

Refactor document loaders into bounded observed-text reads plus pure parsers. Hash
exact bytes from the same read used for parsing; keep registry-level workspace
command validation as well as pure format validation. Dirty baselines now retain
source_sha256. Pure serializers are shared by new-file and checked writers, without
changing authoring schemas or the existing no-overwrite save commands.

Expose scene.save_checked, asset.catalog.save_checked, workspace.save_checked and
project.recent.save_checked with path and optional sha256. Empty/omitted digest
uses a matching document/path observation; no baseline rejects rather than guessing.
project.settings.save uses the active manifest path and the same policy. file.inspect
reports canonical path/size/digest without changing content or observations. It
allows explicit inspection and reviewed conflict resolution, never a force flag.

For a checked replacement, validate/serialize completely, resolve a regular target,
reserve a parent-local .lyko-write-lock directory without waiting, compare bytes,
retain the previous complete bytes under .lyko-backups/<digest>.txt, stage the full
replacement, check bytes again and publish by same-filesystem rename. Editor writers
cooperate on this lock; competing/stale generations return save_busy or
external_change_conflict. Old save commands still refuse any existing destination.
Content-addressed backups are verified before reuse. New lock/backup directories
are owner-private and staged/replacement/backup files preserve basic permission
bits before publication. Existing backup symlinks/files are not adopted as folders.

Success alone updates clean observations/digests. Saving is an explicit external
effect and is not undone; undoing authoring to different contents makes it dirty.
Checked writes cannot be transaction rows. Failed conflicts/locks leave original
bytes, authored state and baselines unchanged. Backups may remain if a later write
fails; leftover locks after process failure need inspection, not unsafe automatic
removal. No fsync/power-loss guarantee, ACL/ownership guarantee or hostile-filesystem
sandbox is claimed. Arbitrary external writers do not honor advisory locks; changes
in the final check/rename window cannot be atomically compared by portable C++.

Tests cover all document kinds, exact raw-source hashes, automatic/explicit tokens,
external modification, unchanged state on conflict, backups/modes, cooperative
process races, lock contention, invalid tokens, source revisions and project-settings
save/close behavior. Existing workspace invalid-keymap regression checks must pass.
Windows and target hardware remain unverified; filesystem portability is a CI gate.
