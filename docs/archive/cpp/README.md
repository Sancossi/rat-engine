# Historical C++ iteration

The [original Sprint 4 roadmap from main `7129902`](architecture-roadmap-main-7129902.md)
is retained byte-for-byte separately from the [later architecture snapshot](architecture-roadmap.md).
Its statements about main and the upcoming Sprint 5 describe that earlier period.

Retired on 2026-09-09 after Rat Expedition moved to Stride/C#. This directory
preserves specifications, plans, schemas, benchmark results, reviews, the previous
game README and library graph. Relative document links follow their new locations.
The documents describe their original implementation and acceptance dates; they
are not instructions or validation evidence for the current Stride game.

The last pre-cleanup snapshot is
[`60fdc55f19889304fc29957fd2124624077aefcd`](https://github.com/sancossi/rat-engine/tree/60fdc55f19889304fc29957fd2124624077aefcd).
All historical source paths, commands and build-artifact paths refer to that
snapshot and its original workspace layout. Source links point to the same Git
baseline. Offline, use `git show 60fdc55f19889304fc29957fd2124624077aefcd:<path>`
or create a separate checkout at that commit to inspect or rebuild it. Git history
has not been rewritten.

The baseline includes `apps`, `src`, `tests`, `benchmarks`, `cmake`, original
`assets`/`data`, CMake configuration, verification/GUI/benchmark/clang scripts and
the historical audit probe. These are retired from the working tree. The current
sprite generator, sprite atlas, Noto Sans font and OFL are self-contained in
`games/rat-expedition/`; credits retain their provenance.

Historical cards, decisions and closed sprints remain in [the vault](../../../vault/).
Engine comparisons and general vault/BMad documents remain outside this archive.
Current instructions are in [the repository README](../../../README.md).
See [the approved cleanup plan](../../cpp-retirement-plan.md) and
[its tracking card](../../../vault/production/tasks/chore-retire-cpp-iteration.md).

Local retained C++ reports and screenshots are copied to `build/cpp-history/`
before confirmed old build directories are removed. The local cleanup manifest
records exact paths and byte counts; these ignored artifacts are not shipped.
