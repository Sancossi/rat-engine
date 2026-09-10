---
type: task
area: Game
status: In progress
task_type: Feature
sprint: Sprint 19
review: Pending
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

Начат срез 1: перенос библиотеки. В отдельной worktree `C:/5_gamedev/rat-engine-dremma` сохранены обе истории: текущий проект и графическая ветка `c74c6e7`. Исходные рабочие копии не изменены. Дальше срез 2 (масштаб/runtime) и срез 3 (игровой квартал); оставшиеся A1/каталог не закрываются автоматически.

## Bugs found

none
