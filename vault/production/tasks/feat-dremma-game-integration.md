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

Повторное review `50aba40` подтвердило isolation fix, но выявило P2 порядка validation: при invalid binding смена/reimport модели B может быть затёрта старым cached Original A. Перенести validation после отслеживания смены component/model; регрессия A → invalid binding → B → repair должна сохранять B.

P2 isolation исправлен в `50aba40` и передан на повторное review: invalid binding исключается только из собственного suppression, valid replacement остаётся подавленной, unrelated geometry видима; диагностика остаётся у неверной привязки, runtime validation строгая. Authoring 21/21. Первый editor harness остановлен безопасно из-за несовпадения формы JSON Position (служебное IsNormalized); это ещё не успешная editor qualification, исправляется проверка явных XYZ.

Review `fb36d6c` — Needs fixes, один P2 в GeometryPreviewProcessor: empty/duplicate AdditionalGeometryId одной привязки выбрасывает исключение при проверке каждого collider и выключает preview несвязанных floor/platform. Обрабатывать неверную привязку отдельно с диагностикой, не подавляя чужую геометрию; проверить сохранение видимости unrelated geometry и подавление другой valid replacement. Сцена, исходная геометрия/порталы и целевые GPU свидетельства получили положительную оценку; финальные gates остаются открыты.

Срез 3 передан на независимое review: `fb36d6c`, Dremma — production start с native графикой и порталом к Courtyard; добавлена many-to-one привязка геометрии. Windows/asset build без ошибок, Core 64/64, Authoring 21/21, Python 25/25, vault/PowerShell parse прошли. Targeted GPU маршрут завершён на кадре 953 (`build/dremma-stair-rails-1`): центральный/смещённый проход арки, crouch, лестница/перила, верхний настил и нижний возврат; после shutdown native lease count 0. Ранее прошли 20 Dremma portal legs и отказ повреждённого candidate с сохранением активной Дрёммы. Editor API, door preview, финальные пакеты и OS gate ещё ожидаются.

Срез 2 опубликован в `rat-engine/main`: `7e1513687d268172c8edfade4fae5c89146c08fb`, remote SHA проверен. Начат срез 3: отдельная редактируемая Dremma.sdscene, старт обычной игры в Дрёмме, связь с Courtyard, мост/лестница/нижняя арка, native art и итоговые проверки. Review Pending относится к новому срезу. OS window-state gate из среза 2 остаётся обязательным до закрытия всей карточки.

Срез 2 принят с явно отложенной до финала OS window-state проверкой. Код `e8f965e` и пакетные свидетельства получили независимое Approved; [отчёт](../../../docs/audits/2026-09-10-dremma-scale-native-runtime.md) зафиксирован в `f12a745`. Core 64/64, Authoring 20/20, Python 25/25; финальные 10 affected/remaining QA сценариев прошли, включая 20 portal legs и сохранение активной сцены при отказах. Unaffected prefix предыдущего пакета проверен отдельно; full verifier PASS не заявляется. Normal ZIP проверен parent вне checkout: `C:/rat-expedition-validation/slice2-normal-20260910-155028-215/result.json`, exit 0. Итоговый полный Release редактора: `C:/5_gamedev/stride/logs/rat-foundation/20260910-154754-185/result.json`, 66,65 s, 5 warnings, 0 errors. Pin `c0b9065d` неизменен. Карточка остаётся открытой: срез 3 и финальная OS проверка ещё обязательны.

Mixed locality исправлен и передан на review в `e8f965e`. Реальный GPU кадр 738 (`build/mixed-release-qualified`): лидер Y2,8643 и первый спутник Y3,5643 Falling/видимы; задний Y3,6 геометрически на верхней опоре и скрыт, bridge-cut/bridge-rail-cut скрыты; маршрут завершён на 839. Его записанный TrailPose.Mode — Falling, что не заменяет геометрическую проверку опоры: требование Grounded отсутствовало в исходном oracle и снято после независимой проверки. Smoke elapsed сохранён 1/30, Core 64/64. Принятие ожидает re-review и оставшиеся пакетные сценарии.

Review `d4fb158` одобрило radius fix/regression, но обнаружило оставшееся неверное требование вертикального интервала 0,7 в mixed observation. Trail spacing измеряется вдоль пути; восстановить проверки высот/режимов разных опор, а точный spacing проверять на существующем прямом grounded segment.

Исправления `d4fb158` переданы на review: ранний mixed capture сохраняет проверку стоящего заднего спутника на скрытом настиле; LocalOcclusion использует TraversalMotor.Radius для ramp support, добавлена регрессия защиты опоры. Core 64/64; пакетные affected и оставшиеся сценарии выполняются.

Остались две проверки среза 2: mixed-companions снимает кадр слишком поздно, когда все уже падают; сохранить исходный смысл теста (падающий лидер и передний спутник видимы, задний ещё стоит на скрываемом верхнем настиле) более ранним наблюдением, не заменять его all-falling oracle. Parent также обнаружил старый radius 0,2 в LocalOcclusion.Supports для ramp при новом физическом radius 0,45; требуется исправление и регрессия защиты опоры.

Body route исправлен в `d972d65`: точное сравнение float на scaled upper exit заменено допуском 0,001 только в наблюдении smoke-маршрута. Реальная опора была достигнута, но тест продолжал движение после неё. Скорости, FSM, геометрия и acceptance assertions не изменены; повторное review и пакетный прогон выполняются.

Пакетный прогон выявил незавершённый body route к 2700 кадрам при exit 0; проверка корректно отклонила результат. Причина и исправление маршрута исследуются. GPU 720/1080, оба camera-edge и wheel прошли. OS window-state gate остаётся pending из-за внешнего foreground; reviewer подтвердил неизменность этой логики и допустимость переноса проверки на финальное закрытие при прохождении остальных assertions. Это не full verifier PASS.

Исправления трёх P2 переданы на повторное review: `9983c9a`, Authoring 20/20, включая valid → invalid → Undo, enabled/disabled occlusion и zero-scale/shear. Дополнительно build-game очищает только локальные Windows Release outputs перед publish для проверки смешивания normal/QA asset intermediates. Финальные normal/QA пакеты и полный verifier ещё выполняются; принятие не объявлено.

Review среза 2 (`9b54bd9`) — Needs fixes, три P2: valid → invalid → Undo не восстанавливает subset в preview; occlusion включает authored disabled ModelComponent; flattening вложенной transform принимает вырожденную матрицу/shear. Исправления и проверки этих сценариев обязательны до принятия. Пакетный verifier дополнительно остановился на startup preflight; причина исследуется.

Срез 2 передан на независимое code review: `9b54bd95fffeb39ed5b1076e70c186f01c2d9c4d`. Core 63/63, Authoring 17/17, Python 25/25 и vault прошли. Native visual resolver/lease, editor bindings/subsets и миграция размеров реализованы; предварительный GPU кадр показал библиотечный фонарь в обычном Courtyard. Normal/QA пакеты и полный verifier выполняются параллельно review и остаются обязательными до принятия.

Срез 1 опубликован в `rat-engine/main`: `53d5ed092a69e9c7215ba5d25adce470acd664fe`, remote SHA проверен. Stride main/pin остаётся `c0b9065d6e902b45d4d3a5c656318c70df53a6f3`. Начат срез 2: отдельное владение native visuals в Windows, authoring bindings/occlusion, масштаб тела 1,8/0,9/radius0,45 и камеры, миграция двух прежних карт ×2,25 при неизменных скоростях и FSM. Новая Dremma-сцена — срез 3. Review Pending относится только к новому срезу.

Срез 1 принят: повторное независимое review `abfb271` — Approved, P2 исправлен, открытых замечаний нет. Итоговый parent Release: `C:/5_gamedev/stride/logs/rat-foundation/20260910-134658-422/result.json`, 61.41 s, 5 warnings, 0 errors; editor `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. Библиотека публикуется в main с сохранением обеих историй. Review Approved относится только к срезу 1; вся карточка остаётся In progress.

Исправление P2 передано на повторное review: `abfb271cdf6092f29963d69d92cb68be4eb1816b`. Qualification `build/mcp/dremma-library-20260910-134227-247` PASS: 73 + 30 MCP вызовов, явные XYZ/RGBA, позиции ±3 до Save/после Undo/Redo/fresh reopen, отрицательная проверка изменения X отклонена. Все 170 исходных файлов сохранены; vault и 25 Python tests прошли. Ожидается повторное review.

Review среза 1 — Needs fixes: P2 в проверке Snapshot. Default JSON сериализует Vector3 без X/Y/Z и Color4 без RGBA, поэтому сравнение могло пропустить перемещение prefab после reopen. Фактические MCP данные показывают правильные ±3 позиции; это пробел acceptance guard. Исправить явные scalar fields и assertions ожидаемых координат, повторить qualification/review. Прочих замечаний нет. Полный Release до исправления теста прошёл: `C:/5_gamedev/stride/logs/rat-foundation/20260910-133759-234/result.json`, 79.20 s, 5 warnings, 0 errors. Публикация ожидает принятия исправления.

Срез 1 передан на независимое review: `16f130e28dcce946938ad286a3dbdf8a814e5c55`. [Отчёт](../../../docs/audits/2026-09-10-dremma-library-integration.md): два текущих editor processes, 74 + 31 MCP вызов, 19 tools, 97 native assets и 73 Resources; реальный FBX reimport 3→7 material slots, новый PNG/source hash, stable IDs/refs, Undo/Redo/Save/fresh reopen PASS. Все 170 исходных файлов библиотеки неизменны. MCP tests, vault и 25 Python tests PASS. Ожидаются review и полный Release; масштаб/runtime и новая карта ещё не реализованы.

Начат срез 1: перенос библиотеки. В отдельной worktree `C:/5_gamedev/rat-engine-dremma` сохранены обе истории: текущий проект и графическая ветка `c74c6e7`. Исходные рабочие копии не изменены. Дальше срез 2 (масштаб/runtime) и срез 3 (игровой квартал); оставшиеся A1/каталог не закрываются автоматически.

## Bugs found

Срез 2: preview Undo после invalid selector, authored Enabled, shear/degenerate transform, normal/QA build cache, QA asset-ID collision, старый radius в ramp support и smoke observation/oracle исправлены и проверены. Открытых кодовых замечаний среза нет; OS foreground gate перенесён на финальную приёмку, не объявлен пройденным.

Срез 1: P2 неполной сериализации XYZ/RGBA в acceptance snapshot исправлен в `abfb271`; отрицательный oracle и повторное review прошли. Открытых дефектов принятой библиотеки нет.
