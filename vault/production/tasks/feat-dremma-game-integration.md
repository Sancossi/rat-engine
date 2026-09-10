---
type: task
area: Game
status: In review
task_type: Feature
sprint: Sprint 19
review: In review
due:
tags: [task, expedition, stride, art]
---

# Дрёмма — библиотека и игровая локация

Intent: Импортировать готовую графику соседнего проекта в текущий Stride, добавить новую стартовую локацию и согласовать масштаб героев 1,8 м без изменения управления и скоростей.

Specification: [Утверждённый план](../../../docs/dremma-game-integration-spec.md).

Origin: [[feat-canal-city-01-foundation]]; [[feat-stride-game-studio-authoring]]; прямой запрос пользователя 2026-09-10.

Acceptance:
- Все 97 готовых native assets и исходники доступны в текущем Authoring; ссылки/Undo/Save/reopen и reimport копий FBX/PNG проверены.
- Native ресурсы используются обычной игрой; рост группы 1,8 м, прежние скорости/FSM, сохранённая крупность кадра; старые карты адаптированы.
- Dremma — стартовая карта с новым маршрутом и двусторонней связью с Courtyard; мост, лестницы, коллизии, occlusion и спутники проверены.
- Проверенные normal/QA ZIP, независимое review, полный Release редактора и публикация каждого принятого среза в main.

## Resolution

Исправление P2 передано на повторное review: `abfb271cdf6092f29963d69d92cb68be4eb1816b`. Qualification `build/mcp/dremma-library-20260910-134227-247` PASS: 73 + 30 MCP вызовов, явные XYZ/RGBA, позиции ±3 до Save/после Undo/Redo/fresh reopen, отрицательная проверка изменения X отклонена. Все 170 исходных файлов сохранены; vault и 25 Python tests прошли. Ожидается повторное review.

Review среза 1 — Needs fixes: P2 в проверке Snapshot. Default JSON сериализует Vector3 без X/Y/Z и Color4 без RGBA, поэтому сравнение могло пропустить перемещение prefab после reopen. Фактические MCP данные показывают правильные ±3 позиции; это пробел acceptance guard. Исправить явные scalar fields и assertions ожидаемых координат, повторить qualification/review. Прочих замечаний нет. Полный Release до исправления теста прошёл: `C:/5_gamedev/stride/logs/rat-foundation/20260910-133759-234/result.json`, 79.20 s, 5 warnings, 0 errors. Публикация ожидает принятия исправления.

Срез 1 передан на независимое review: `16f130e28dcce946938ad286a3dbdf8a814e5c55`. [Отчёт](../../../docs/audits/2026-09-10-dremma-library-integration.md): два текущих editor processes, 74 + 31 MCP вызов, 19 tools, 97 native assets и 73 Resources; реальный FBX reimport 3→7 material slots, новый PNG/source hash, stable IDs/refs, Undo/Redo/Save/fresh reopen PASS. Все 170 исходных файлов библиотеки неизменны. MCP tests, vault и 25 Python tests PASS. Ожидаются review и полный Release; масштаб/runtime и новая карта ещё не реализованы.

Начат срез 1: перенос библиотеки. В отдельной worktree `C:/5_gamedev/rat-engine-dremma` сохранены обе истории: текущий проект и графическая ветка `c74c6e7`. Исходные рабочие копии не изменены. Дальше срез 2 (масштаб/runtime) и срез 3 (игровой квартал); оставшиеся A1/каталог не закрываются автоматически.

## Bugs found

none
