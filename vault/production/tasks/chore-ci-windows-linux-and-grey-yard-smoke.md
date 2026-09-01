---
type: task
area: Engine
status: Not started
task_type: Chore
sprint: Sprint 7
due:
tags: [task]
---

# chore: CI Windows/Linux and grey_yard smoke

Intent: чистый checkout собирается без ручных шагов. Этап 0 [architecture roadmap](../../../docs/architecture-roadmap.md). Взято в [[Sprint 7 — Engine architecture]].

Acceptance: GitHub Actions (Windows + Linux) собирают `rat_core`, editor и Catch2; все headless-тесты зелёные; отдельный smoke грузит `grey_yard` без окна. Verify: PR check + `ctest`.

Depends: none. Next: [[chore: rat_core isolation from platform graphics]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).
