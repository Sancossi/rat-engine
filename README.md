# Rat Expedition

Rat Expedition is a Windows game developed in Stride/C#. The production start is
the native Dremma city scene, with a traversable bridge, lower arch passage,
stairs, party movement, and portals to the retained Courtyard and Sluice maps.
Start with the
[game and controls](games/rat-expedition/README.md), [game design](vault/game/GDD.md)
and [pinned Stride source workflow](docs/stride-source-workflow.md).

## Build and verify

Prepare the pinned Stride checkout and local package feed using the source
workflow. The tested SDK is .NET 10.0.300; the exact engine revision and native
build requirements are recorded in [engine.lock.json](tools/stride/engine.lock.json).
The upstream source lives in a separate checkout, outside this repository.

From the repository root on Windows:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/build-game.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/verify-game.ps1 -PackageZip <path-to-new-ZIP>
```

The build runs Core scenarios and creates a self-contained Release package under
`build/stride-game/<timestamp>/`. Verification extracts that ZIP outside the
checkout and exercises the real executable, resources, camera and traversal.
Launch the packaged `Rat.Expedition.Windows.exe` to play.

To rebuild the full Release Game Studio editor:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/build.ps1
```

The wrapper prints the actual editor path and build evidence. An editor build
does not establish GUI or game acceptance. Python repository checks also run in
[CI](docs/continuous-integration.md):

```text
python scripts/check_vault.py
python -m unittest discover -s scripts/tests
```

## Project knowledge and history

Open [vault/](vault/) in Obsidian for product decisions and the work queue.
Engineering specifications live in `docs/`; game source and content live in
`games/rat-expedition/`. Follow [AGENTS.md](AGENTS.md) for execution and review.

The previous C++ engine/editor has been retired. Its
[documentation archive](docs/archive/cpp/README.md) preserves specifications,
reviews and the exact Git baseline for recovering source, maps and build scripts.
Historical cards and closed sprints remain in the vault.
