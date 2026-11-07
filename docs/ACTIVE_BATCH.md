# Active implementation batch: checklist steps 11–20

User request: implement the next ten steps after step 10. Continue this batch
across sessions/compactions; do not treat one slice as completion of the request.
No engine access, paid services, merges or public release are authorized.

| Step | Scope | State |
| --- | --- | --- |
| 11 | Persisted-document dirty baselines, project settings and UI status | Implemented locally; full 43/34 checks pass; branch codex/standalone-editor-dirty-state |
| 12 | Safe existing-file saves, external-change checks and conflicts | In progress: single-read observations and text serializer extraction |
| 13 | Autosave/recovery snapshots and crash recovery | Pending |
| 14 | Schema migrations, backups and compatibility errors | Pending |
| 15 | Multiple open documents and clear history ownership | Pending |
| 16 | Long-job progress/cancellation and atomic commits | Pending |
| 17 | Extensible typed component/property schema | Pending |
| 18 | Component/property command editing | Pending |
| 19 | Schema-driven nested inspector and references | Pending |
| 20 | Multi-selection and atomic selected-entity operations | Pending |

Each item requires implementation, appropriate checks, ADR/STATUS updates and
reviewable private commits/branches. Checkboxes in DELIVERY_CHECKLIST own completion
counts; keep this queue consistent with it. Do not claim target-hardware/Windows
verification without evidence. Preserve the existing private review stack.
