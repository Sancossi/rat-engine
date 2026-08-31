---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: PlaySE event command

Intent: opcode события (GPP Bytecode) постит в очередь [[feat: Audio play-queue stub]]. Зависит от audio stub; бэкенд звука не обязателен (null sink ок).

Acceptance: JSON-команда в схеме + runtime постит `PlaySfx`; headless тест без устройства.
