---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Audio play-queue stub

Intent: GPP Event Queue для звука + явный интерфейс `Audio` (не locator, не синглтон), чтобы геймплей постил PlaySfx без знания бэкенда. Контекст: сверка [[Game Programming Patterns]] (2026-08-31); продукт — [[Audio Direction]]. Выбор miniaudio/OpenAL — отдельный ADR позже. Opcode `PlaySE` в событиях — follow-up.

Acceptance: очередь `PlaySfx` / `PlayMusic` / `Stop`; null/log sink; drain раз в кадр из shell; геймплей не знает бэкенд.

## Resolution

`QueuedAudio` + `AudioSink` (null / log / recording) in `rat_core`. `play_*` / `stop` only enqueue; `drain()` applies FIFO then clears. `EditorApp` owns `LogAudioSink` wrapping `Logger` and drains once per display frame (after `update_simulation`). Verify: `.\build\tests\rat_tests.exe "[audio]"`. PlaySE: [[feat: PlaySE event command]].

Follow-up: [[chore: audio queue drain snapshot and header include]]

## Bugs found

none (продуктовых). Minor ревью: in-place `drain`, `audio.hpp` includes `log.hpp` — см. follow-up.
