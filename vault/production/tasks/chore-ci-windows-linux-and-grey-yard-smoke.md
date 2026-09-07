---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 7
due:
tags: [task]
---

# chore: CI Windows/Linux and grey_yard smoke

Intent: чистый checkout собирается без ручных шагов. Этап 0 [architecture roadmap](../../../docs/architecture-roadmap.md). Взято в [[Sprint 7 — Engine architecture]].

Acceptance: GitHub Actions (Windows + Linux) собирают `rat_core`, editor и Catch2; все headless-тесты зелёные; отдельный smoke грузит `grey_yard` без окна. Verify: PR check + `ctest`.

Depends: none. Next: [[chore-rat-core-isolation-from-platform-graphics|chore: rat_core isolation from platform graphics]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).

## Resolution

GitHub Actions Windows+Linux: Ninja Release, `rat_core` / `rat-editor` / `rat_tests`, `ctest`, then `rat_tests "[smoke]"` (grey_yard via `load_map_from_file`, no window). Linux editor compiles with GLFW X11 nwh/ndt. `GLFW_BUILD_WAYLAND OFF` so Ubuntu configure does not require wayland pkg-config. Verify: `ctest` + `[smoke]`; first Actions run still the live check. Review: Approved after Wayland fix.

## Bugs found

none.

