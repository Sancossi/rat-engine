---
name: rat-build-verify
description: Configure, build, and verify rat-engine on Windows or Linux and report precise test and editor artifacts. Use for rat-engine build failures and stage acceptance.
---

# Build and verify rat-engine

Read the repository `AGENTS.md`. Run `scripts/verify.ps1` on Windows; it discovers
Visual Studio through vswhere and imports its x64 environment without hardcoded
installation versions. `-Preset dev-debug` selects Debug; default is dev-release.
The wrapper prints its binary directory and editor executable after successful checks.

On Linux, use the matching configure/build/test presets and run
`python3 scripts/check_vault.py` and `python3 -m unittest discover -s scripts/tests`.
Install dependencies listed in README before configure. First configure needs network.

Existing `build/` may use a different generator or toolchain. Presets isolate outputs
under `build/dev-release` and `build/dev-debug`; never clear another build to fix this.
For narrow iteration use the relevant existing Catch2 tags, then run full Release
verification at stage close. Record commands, exit codes, test count, executable path,
and anything not run. A successful build does not prove interactive GUI behavior.
