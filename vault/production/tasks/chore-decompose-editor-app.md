---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 7
due:
tags: [task]
---

# chore: Decompose EditorApp

Intent: `EditorApp` только создаёт зависимости. `GlfwHost` — окно и сырой ввод; `FrameCoordinator` — фазы кадра; `EditorDocument` — карта, selection, dirty, history; панели шлют команды документа, не мутируют runtime. `SimulationSession` владеет gameplay. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: в `EditorApp` нет игровой физики; панели не зовут mutation `EventRuntime`; Play/Edit не уничтожает authoring state; операции документа тестируются без окна и ImGui.

Depends: [[feat-map-document-and-runtime-map|feat: MapDocument and RuntimeMap compile]]. Next: [[define-asset-folder-load-stub|feat: Asset registry and first load vertical]].

Origin: merged `docs/archive/cpp/architecture-roadmap.md` (2026-09-01).

## Resolution

`GlfwHost`, `FrameCoordinator`, `EditorDocument` (`rat_editor_logic`) и панели blocker/terrain/event. `EditorApp` — composition root + Play chrome; tick в `SimulationSession`. Документные операции без окна: `[editordoc]`. Verify: `.\build\tests\rat_tests.exe "[editordoc]"` и `ctest`. Review: Approved.

## Bugs found

none.

