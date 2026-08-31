---
type: task
area: Engine
status: In progress
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: input sequence probe review polish

Intent: закрыть minor из ревью [[feat: Headless input sequence probe]] (Approved, не блокеры): ошибка записи snapshot не глотается молча; Edit либо snap-to-ground как редактор, либо убрать из API; temp-каталог теста уникален и чистится.

Acceptance: `write_snapshot_each_step` даёт сигнал, если файлов нет; тест не шарит `temp/rat-input-sequence-probe`; поведение Edit задокументировано или совпадает с редактором.

Origin: [[feat: Headless input sequence probe]]

## Resolution

`InputSequenceResult` now has `snapshots_written` and `snapshot_error`. Failed `create_directories` / `write_debug_snapshot` set the error and leave the count at 0 instead of looking successful. Probe snapshot tests use a unique temp path and `remove_all` in teardown. Edit mode: documented as no snap-to-ground (Play default; editor-only snap).

## Bugs found

none.

Verify: `.\build\tests\rat_tests.exe "[probe]"`
