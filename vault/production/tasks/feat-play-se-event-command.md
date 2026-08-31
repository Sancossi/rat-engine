---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: PlaySE event command

Intent: opcode события (GPP Bytecode) постит в очередь [[feat: Audio play-queue stub]]. Зависит от audio stub; бэкенд звука не обязателен (null sink ок).

Acceptance: JSON-команда в схеме + runtime постит `PlaySfx`; headless тест без устройства.

## Resolution

Opcode `play_se` (`CommandOp::PlaySE`) parses `id` into `Command::text`, serializes the same shape, and `EventRuntime::exec_command` posts `Audio::play_sfx` when `set_audio` is non-null (no `drain()` in the opcode; null audio is a consumed no-op). `EditorApp` injects `QueuedAudio` after construction. Verify: `.\build\tests\rat_tests.exe "[playse]"`.

Follow-up: [[chore: PlaySE review test polish]]

## Bugs found

none (продуктовых). Minor ревью: слабый substring `"id"`, нет теста на пустой cue — см. follow-up.
