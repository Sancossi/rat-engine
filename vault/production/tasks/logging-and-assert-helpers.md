---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 4
roadmap: Engine bootstrap
due:
tags: [task]
notion_id: 3ccf3827-36cc-814a-a5f0-ed62907089da
---

# Logging and assert helpers

Intent: примитив лога в `rat_core` (уровни, sink в файл/`stderr`), чтобы агент и человек читали один поток. Сейчас `warnings_` только у Parallel. Не Service Locator: sink передаётся явно или `EditorApp` владеет. Контекст: [[Agent Debug]], [[Engine Vision]] («ошибки видимы в debug»).

Acceptance: `rat::log(level, channel, msg)`; файл в working dir или override; assert в debug + лог в release на failure path; тест пишет в memory sink.

## Resolution

`rat::Logger` + `LogSink` (memory / file / stderr / tee). Свободная `rat::log(logger, level, channel, msg)` — без синглтона; `EditorApp` владеет file+stderr. Файл: `rat.log` или `RAT_LOG_PATH`. `rat::check` / `RAT_ASSERT`. Тесты: `[log]`.

## Bugs found

none.
