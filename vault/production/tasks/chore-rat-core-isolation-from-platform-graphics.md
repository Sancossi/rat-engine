---
type: task
area: Engine
status: Not started
task_type: Chore
sprint: Sprint 7
due:
tags: [task]
---

# chore: rat_core isolation from platform graphics

Intent: зафиксировать граф зависимостей. Gameplay/`rat_core` не линкует GLFW, ImGui, bgfx, Win32. Сейчас `rat_core` уже только `nlohmann_json` — нужна проверка, чтобы это не разъехалось. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: CMake/CI падает, если `rat_core` тянет GLFW/ImGui/bgfx; в README или `cmake/` виден граф библиотек. Editor остаётся в `apps/editor` + `rat_engine`.

Depends: [[chore: CI Windows/Linux and grey_yard smoke]]. Next: [[feat: SimulationSession unified tick]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).
