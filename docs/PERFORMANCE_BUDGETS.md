# Performance budgets and measurement procedure

Status: defined locally for Joshua's review (checklist step 7). These are proposed
acceptance targets, not claims that the current editor or target hardware passes.
The non-negotiable smooth 60 fps goal remains. Actual hardware work is steps 76–77.

## Workloads and release targets

Use a Release build without sanitizer/tidy instrumentation for acceptance runs.
Record commit, compiler/options, OS, display resolution/scale/refresh, CPU/RAM/GPU,
GPU driver, storage, power/thermal mode and the workload. Measure the RTX 4070 PC
and MSI Claw separately, including the handheld's agreed practical power setting.
Do not compare cloud software-renderer results to those hardware acceptance runs.

| Metric | Proposed budget | Workload / measurement boundary |
| --- | --- | --- |
| Headless readiness | p95 <= 500 ms | Fresh lyko process through first successful scene.query response, including client JSON decode; 20 launches; filesystem cache state recorded |
| Desktop usable startup | p95 <= 2 s warm; <= 5 s after reboot | Process start through first interactive rendered frame with initial snapshot/palette available; 20 warm launches and 3 reboot-first launches, reported separately |
| Interactive frame pacing | p95 <= 16.67 ms; p99 <= 33.34 ms; no avoidable UI-main-thread stalls > 50 ms | Three 60 s runs each: idle, scene-tree/asset-table scrolling, palette/property editing and layout changes; report render work separately from pacing sleep |
| Query round trip | p95 <= 50 ms, p99 <= 100 ms | Warm persistent headless client, 1000-entity scene.tree and 4096-entry asset.list; include JSON decode and transport, exclude fixture setup |
| Rename + undo pair | p95 <= 100 ms | Same scene/catalogue, 100 repetitions after 10 warmups; validate responses and keep history size stable by undoing each rename |
| Action feedback while work is pending | <= 100 ms to show accepted/busy state | Desktop command submission through next feedback frame, including long source/import/build jobs; completion time is a separate metric |
| Resident memory | <= 512 MiB small; <= 1 GiB large | Standalone desktop/mock after 60 s idle and a 10 min representative authoring session; small fixture project vs generated 1000 entities/4096 metadata assets; report startup peak and stabilized value |
| Memory growth | <= 5% unexplained growth over five identical cleared/reopened workload cycles | Keep project size/history policy fixed; separate retained history/caches from leaks; report absolute MiB too |

GPU/resource budgets must be added when the approved renderer/importer exists.
Do not apply mock resident-memory numbers to real texture/mesh residency. Report
source validation/import/build durations per input size rather than imposing one
fixed completion deadline on arbitrarily large jobs. Their UI responsiveness and
cancellation latency still need budgets and measurements as jobs are implemented.

## Repeatable headless baseline

The opt-in standard-library harness creates temporary original fixtures, checks
successful responses, warms up the session and prints JSON. It uploads nothing,
opens no external service, and does not add a timing-sensitive CI pass/fail test.

```bash
python3 editor/tools/benchmark-headless.py editor/build/desktop/lyko --build-label Debug-local > /tmp/lyko-headless-baseline.json
```

A headless Release build can be prepared with the existing dev preset in its own
build directory (from editor/):

```bash
cmake --preset dev -B build/benchmark-release -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTING=OFF
cmake --build build/benchmark-release -j 3
python3 tools/benchmark-headless.py build/benchmark-release/lyko --build-label Release-local
```

Use the Release executable and matching label for target acceptance runs. The
harness samples startup and client round trips; Linux /proc reports current RSS
and process high-water RSS where available. RSS is not a precise allocation/leak
measurement. Missing memory data is null, never an inferred pass. Startup launches
are fresh processes with uncontrolled disk cache, not true cold-start measurements.
The rename/undo pair has one timing sample per complete pair. The harness does not
measure desktop frames, desktop memory, accessibility or real runtime behavior.

## Desktop and hardware procedure

1. Load valid/project fixtures for the small case; generate large fixtures into a
   fresh folder and load their scene/catalogue for the large case. Record the exact
   datasets and fixed operations used. Use mock backend until integration is approved.
2. Record monotonic frame-start, render-work-end and present/pacing-end timestamps
   for each measured frame. Capture submission and feedback-frame timestamps for
   action latency. If this instrumentation is not available, mark these metrics
   unmeasured; a screenshot or frame-cap constant is not evidence of 60 fps.
3. Run idle and interaction scenarios three times for 60 s each after a 10 s warmup.
   Use a fixed input trace for repeatability; distinguish injected inputs from real
   touch/controller trials. Retain p50/p95/p99/max and counts of missed frame budgets.
4. For desktop startup, measure process launch through the first rendered usable
   frame and readiness response. Keep warm and reboot-first trials separate; avoid
   privileged cache flushing or changes to shared machines merely to manufacture
   a cold run. Missing reboot-first trials remain unverified.
5. Sample process RSS/working set and peak usage with OS tooling during the fixed
   10 min scenario, then repeat the same close/reopen/clear cycle five times.
   On Windows record working set/private bytes and tooling/version; on Linux use
   /proc or a profiler. Distinguish OS metric definitions and debugger overhead.
6. Compare each device/scenario to the table, retain raw local timing/memory data
   and summarize failures with reproduction steps. A regression is actionable even
   if a broader milestone is still open. Artifact uploads remain opt-in and small.

Do not mark hardware/release validation complete until those measurements exist.
The current desktop has no exported per-frame trace yet; adding instrumentation
and capturing target-device runs remain implementation/validation work. Screen
readers, input usability and the real renderer need separate acceptance evidence.


## Initial local baseline (2026-11-06)

Code/fixture checkout: b787395 (runtime implementation checkpoint 770f17e).
Linux x86-64 cloud, Release headless mock, 5 logical CPUs. Twenty fresh-process
launches and 100 samples per operation after ten warmups. Filesystem cache was
uncontrolled; no real GPU/render/input-device work was measured.

| Metric | Observed p95 | Observed p99 |
| --- | --- | --- |
| First headless response | 3.10 ms | 3.14 ms |
| 1000-entity scene tree | 5.96 ms | 9.90 ms |
| 4096-entry asset list | 14.61 ms | 17.16 ms |
| Rename + undo pair | 8.00 ms | 18.24 ms |

Sampled process RSS was 9,441,280 bytes, high-water RSS 11,767,808 bytes. This is
headless/mock process memory, not the desktop residency budget or a leak test.
The script successfully ran in both Debug and Release configurations. Debug asset
list timing was substantially higher; Release results are the relevant local
baseline. Raw local report: /tmp/lyko-headless-release-baseline-v1.json (ephemeral;
rerun the documented command to reproduce rather than depending on that path).
These observations do not verify target hardware, desktop frame pacing or release
acceptance. Full hardware measurements remain outstanding.
