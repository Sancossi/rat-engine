---
name: rat-build-verify
description: Build and verify Rat Expedition and its pinned Stride Game Studio on Windows, reporting precise test and editor artifacts. Use for build failures and stage acceptance.
---

# Build and verify Rat Expedition

Read `AGENTS.md`, the affected card and `docs/stride-source-workflow.md`.
`tools/stride/engine.lock.json` pins the upstream checkout, editor configuration
and native requirements. Preserve the game package cache, local feed and upstream
native tools; do not upgrade dependencies or clear caches to fix ordinary builds.

From the repository root on Windows run
`powershell -ExecutionPolicy Bypass -File scripts/stride/build-game.ps1`.
It checks the pinned source/feed, restores/builds, runs Core scenarios and publishes
a self-contained Release ZIP under a new `build/stride-game/<timestamp>/` directory.
Run `scripts/stride/verify-game.ps1 -PackageZip <new-ZIP>` for executable acceptance
outside the checkout, resource failures, both window sizes, camera and body traversal.
For narrow Core iteration use
`dotnet run --project games/rat-expedition/Rat.Expedition.Core.Tests -c Release`.

Run `python scripts/check_vault.py` and
`python -m unittest discover -s scripts/tests` for repository checks.
The parent rebuilds full Release Game Studio at stage close with
`powershell -ExecutionPolicy Bypass -File scripts/stride/build.ps1`.

Record commands, exit codes, scenario count, package/executable paths and actual
manifest/log locations. Separate Core tests, real executable captures, manual
playtests and editor builds. Neither headless success nor an editor build proves
interactive GUI behavior. Do not claim remote CI or a clean-machine test without
observing it. Retired C++ build instructions live in `docs/archive/cpp/` only.
