---
type: task
area: Engine
status: In progress
task_type: Research
sprint: Sprint 7
due:
tags: [task]
---

# research: Audio backend ADR

Intent: выбрать реализацию под интерфейс [[feat: Audio play-queue stub]] (miniaudio / OpenAL / иное). Не Service Locator: провайдер по-прежнему владеет `EditorApp`.

Acceptance: ADR Accepted с Windows-primary и способом подмены sink в тестах. Клипы в runtime — `AssetId`, не сырые пути.

Depends: [[feat: Input rebind and gamepad]]. Next: [[feat: Audio backend implementation]]. Взято в [[Sprint 7 — Engine architecture]].

Origin: [[feat: Audio play-queue stub]] + merged `docs/architecture-roadmap.md` (2026-09-01).
