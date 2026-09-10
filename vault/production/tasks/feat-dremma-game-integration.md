---
type: task
area: Game
status: In progress
task_type: Feature
sprint: Sprint 19
review: Needs fixes
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

Review среза 2 (`9b54bd9`) — Needs fixes, три P2: valid → invalid → Undo не восстанавливает subset в preview; occlusion включает authored disabled ModelComponent; flattening вложенной transform принимает вырожденную матрицу/shear. Исправления и проверки этих сценариев обязательны до принятия. Пакетный verifier дополнительно остановился на startup preflight; причина исследуется.

Срез 2 передан на независимое code review: `9b54bd95fffeb39ed5b1076e70c186f01c2d9c4d`. Core 63/63, Authoring 17/17, Python 25/25 и vault прошли. Native visual resolver/lease, editor bindings/subsets и миграция размеров реализованы; предварительный GPU кадр показал библиотечный фонарь в обычном Courtyard. Normal/QA пакеты и полный verifier выполняются параллельно review и остаются обязательными до принятия.

Срез 1 опубликован в `rat-engine/main`: `53d5ed092a69e9c7215ba5d25adce470acd664fe`, remote SHA проверен. Stride main/pin остаётся `c0b9065d6e902b45d4d3a5c656318c70df53a6f3`. Начат срез 2: отдельное владение native visuals в Windows, authoring bindings/occlusion, масштаб тела 1,8/0,9/radius0,45 и камеры, миграция двух прежних карт ×2,25 при неизменных скоростях и FSM. Новая Dremma-сцена — срез 3. Review Pending относится только к новому срезу.

Срез 1 принят: повторное независимое review `abfb271` — Approved, P2 исправлен, открытых замечаний нет. Итоговый parent Release: `C:/5_gamedev/stride/logs/rat-foundation/20260910-134658-422/result.json`, 61.41 s, 5 warnings, 0 errors; editor `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. Библиотека публикуется в main с сохранением обеих историй. Review Approved относится только к срезу 1; вся карточка остаётся In progress.

Исправление P2 передано на повторное review: `abfb271cdf6092f29963d69d92cb68be4eb1816b`. Qualification `build/mcp/dremma-library-20260910-134227-247` PASS: 73 + 30 MCP вызовов, явные XYZ/RGBA, позиции ±3 до Save/после Undo/Redo/fresh reopen, отрицательная проверка изменения X отклонена. Все 170 исходных файлов сохранены; vault и 25 Python tests прошли. Ожидается повторное review.

Review среза 1 — Needs fixes: P2 в проверке Snapshot. Default JSON сериализует Vector3 без X/Y/Z и Color4 без RGBA, поэтому сравнение могло пропустить перемещение prefab после reopen. Фактические MCP данные показывают правильные ±3 позиции; это пробел acceptance guard. Исправить явные scalar fields и assertions ожидаемых координат, повторить qualification/review. Прочих замечаний нет. Полный Release до исправления теста прошёл: `C:/5_gamedev/stride/logs/rat-foundation/20260910-133759-234/result.json`, 79.20 s, 5 warnings, 0 errors. Публикация ожидает принятия исправления.

Срез 1 передан на независимое review: `16f130e28dcce946938ad286a3dbdf8a814e5c55`. [Отчёт](../../../docs/audits/2026-09-10-dremma-library-integration.md): два текущих editor processes, 74 + 31 MCP вызов, 19 tools, 97 native assets и 73 Resources; реальный FBX reimport 3→7 material slots, новый PNG/source hash, stable IDs/refs, Undo/Redo/Save/fresh reopen PASS. Все 170 исходных файлов библиотеки неизменны. MCP tests, vault и 25 Python tests PASS. Ожидаются review и полный Release; масштаб/runtime и новая карта ещё не реализованы.

Начат срез 1: перенос библиотеки. В отдельной worktree `C:/5_gamedev/rat-engine-dremma` сохранены обе истории: текущий проект и графическая ветка `c74c6e7`. Исходные рабочие копии не изменены. Дальше срез 2 (масштаб/runtime) и срез 3 (игровой квартал); оставшиеся A1/каталог не закрываются автоматически.

## Bugs found

Срез 1: P2 неполной сериализации XYZ/RGBA в acceptance snapshot исправлен в `abfb271`; отрицательный oracle и повторное review прошли. Открытых дефектов принятой библиотеки нет.
