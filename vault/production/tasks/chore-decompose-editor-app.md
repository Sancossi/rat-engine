---
type: task
area: Engine
status: Not started
task_type: Chore
sprint: Sprint 7
due:
tags: [task]
---

# chore: Decompose EditorApp

Intent: `EditorApp` только создаёт зависимости. `GlfwHost` — окно и сырой ввод; `FrameCoordinator` — фазы кадра; `EditorDocument` — карта, selection, dirty, history; панели шлют команды документа, не мутируют runtime. `SimulationSession` владеет gameplay. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: в `EditorApp` нет игровой физики; панели не зовут mutation `EventRuntime`; Play/Edit не уничтожает authoring state; операции документа тестируются без окна и ImGui.

Depends: [[feat: MapDocument and RuntimeMap compile]]. Next: [[feat: Asset registry and first load vertical]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).
