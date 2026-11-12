# Consolidation evidence — 12 November 2026

All 36 tips below are ancestors of ea670eabd800d37f281a90cfde6386cd28a065c3.
The original 26-branch stack and ten later implementation branches are preserved.
No branch was deleted, rebased or force-pushed. main remains README-only.
This proves commit inclusion, not build correctness or code/ADR approval.

| Branch | Inspected commit |
| --- | --- |
| codex/standalone-editor-active-roots | a4e9600155d627dddec84d7ce3820658860137f5 |
| codex/standalone-editor-asset-instantiation | cdb6be60fcaea4102a04276669a9151f571b1471 |
| codex/standalone-editor-assets | 62b261b39125306e1bb624a5635682654a7a69ae |
| codex/standalone-editor-async | 7804e6d747f9df7547e9dc658ef081dfdcac5624 |
| codex/standalone-editor-audit-reports | 2bfbccd47d70580f12478d8953fc16a53861dc1d |
| codex/standalone-editor-checked-saves | ea670eabd800d37f281a90cfde6386cd28a065c3 |
| codex/standalone-editor-core | 38e8e1b1b821f43f474c924649d3a2ada5bcb322 |
| codex/standalone-editor-core-utf8 | 98b9732329e7cc08743ec69405a61b7e5d7ff53a |
| codex/standalone-editor-delivery-checklist | 124b0f6ccda376a916f5c2af2ff7f0fafa35d464 |
| codex/standalone-editor-delivery-milestones | 1b38fb262a0f45b347534bd4dfcf588b513eab56 |
| codex/standalone-editor-desktop | c0d47c15f1821d792b58630d1da4f5aba212d926 |
| codex/standalone-editor-dirty-state | c892e776a86f3711460e65b0435f19cd03ce6768 |
| codex/standalone-editor-docking | 4f650e9b6bdac0e21cd6495df7bd9e5f60ed4035 |
| codex/standalone-editor-entity-actions | 376147a705f9b689692c35c81498ba515b710e93 |
| codex/standalone-editor-form-feedback | a5c5ace3be421bec0ab9cdac6ddd7c63613cb7cd |
| codex/standalone-editor-forms | 824ca17b8393de2df1ead2ad17c0092de54071c0 |
| codex/standalone-editor-layout-forms | 57fdfef7c5303521eec8a3c6e4a5f3ebda40f520 |
| codex/standalone-editor-performance-budgets | cb73a8c4f7cf336b23335b810fb9de50e502fee5 |
| codex/standalone-editor-prefabs | 822824b3adccb24e61932d229b775b3cc32e6584 |
| codex/standalone-editor-preserve-world | b1254845d879267492948b963da9afdb7fd40489 |
| codex/standalone-editor-project-context | c49c13366a6b1fa66f4a6e20362072d29c0b38ad |
| codex/standalone-editor-project-creation | 94ebc55019184a81fe01d1701d7fbc06f1be696d |
| codex/standalone-editor-project-fixtures | b787395536066dc4b34c610d7a3f456179cbfbc0 |
| codex/standalone-editor-project-manifest | 770f17e4ce884a144969c91eb2c82a02b5adc438 |
| codex/standalone-editor-project-recents | ff0e07323e10782d46913796079304d9326bbba6 |
| codex/standalone-editor-scene-tree | 6b71a5eb6a02c44de8621a076789938e2a0c62a9 |
| codex/standalone-editor-shortcuts | 8d79d23b0df85a99d5b69b372455a1e2b6379e0e |
| codex/standalone-editor-source-audit | d72a17c0acc42cbc892816f7b63df7ba228c57cf |
| codex/standalone-editor-source-fingerprints | 4fc7bdc8bc7777663290969fa3bed97fb3776343 |
| codex/standalone-editor-source-validation | b2746d79c19085c820307de77e067ea93b15649c |
| codex/standalone-editor-transactions | 666bc49ebc9211cef6be718d112cd30a57e16978 |
| codex/standalone-editor-transforms | ca6c33c96a8e7a0c97c017bbe12aba9cdcfdc519 |
| codex/standalone-editor-vendor | 79417a26d910778459379e54398f959e3c15d767 |
| codex/standalone-editor-visual-polish | 35dfa7fdf6ba1755797a3365aad52416b5977d2b |
| codex/standalone-editor-workspace | 668204c597a68957f88d6b648c2401a641cb4bc3 |
| codex/standalone-editor-world-transforms | 6fac4c61453fc1765aedf066066b6f31efeae193 |

## Validation handoff

1. Obtain explicit owner approval before merging CI-only PR #1.
2. With editor.yml available on main, dispatch against codex/consolidated-validation-2026-11-12.
3. Record the selected head SHA and run/job URLs. Workflow runs headless and desktop on Ubuntu 24.04 and Windows 2022, then Linux ASan/UBSan, clang-tidy and targeted TSan.
4. Diagnose actual failures on isolated branches; retry only affected checks when possible.
5. Keep implementation integration PR draft until regression evidence and review are complete.

No hosted workflow was run. No fresh local build was possible: CMake/Ninja/compiler unavailable on PATH. Historical 44/44 desktop and 35/35 sanitizer counts are unverified here. Checklist stays 8/93.

The old REVIEW_STACK.md API-policy and runner notes describe an earlier environment. The current GitHub connector can create branches and PRs, but offers no new-run dispatch action; local gh is signed out.
