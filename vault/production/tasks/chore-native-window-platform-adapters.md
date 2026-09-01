---
type: task
area: Engine
status: Not started
task_type: Chore
sprint: Sprint 7
due:
tags: [task]
---

# chore: NativeWindow platform adapters

Intent: спрятать Win32 за минимальным `NativeWindow`; окно для Windows и Linux; часы ОС и файлы — узкие интерфейсы. Через границу симуляции по-прежнему только `InputFrame`. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: `rat_core` / `SimulationSession` не видят HWND/Win32; editor host на Linux собирается (хотя бы окно + ввод); файловые операции не размазаны по gameplay.

Depends: [[feat: RenderWorld packets and instrumentation]]. Next: [[feat: Input rebind and gamepad]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).
