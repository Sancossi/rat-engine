---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Gameplay notify observer

Intent: тонкая шина сигналов (GPP Observer) для UI/аудио: item picked, dialog shown, landed — без подмены [[Event System]]. Брать когда [[feat: Audio play-queue stub]] уже есть и геймплей иначе тянет `Audio*` вглубь.

Acceptance: подписка/пост без синглтона; RM-команды не ходят через шину; тесты без окна.

## Resolution

`GameplayNotifyBus` in `rat_core`: `subscribe` / `post` FIFO, no singleton. `EventRuntime::set_notify` (nullable, same inject as `set_audio`) posts `DialogShown` after `ShowText` and `ItemPicked` after `ChangeItems` with `item_delta > 0`. `PlayerFrameResult::landed` is true on airborne→grounded in `integrate_player_frame_surface`; `EditorApp` posts `Landed` from that flag and owns the bus. Verify: `.\build\tests\rat_tests.exe "[notify]"`.

## Bugs found

none.

