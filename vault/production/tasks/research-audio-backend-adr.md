---
type: task
area: Engine
status: Not started
task_type: Research
sprint:
due:
tags: [task]
---

# research: Audio backend ADR

Intent: выбрать реализацию под интерфейс [[feat: Audio play-queue stub]] (miniaudio / OpenAL / иное). Не Service Locator: провайдер по-прежнему владеет `EditorApp`.

Acceptance: ADR Accepted с Windows-primary и способом подмены sink в тестах.
