# P1.3 — session, ярусы и представление Stride

Контракт: [P1.3](../stride-session-layered-traversal-spec.md), [полный P1](../rat-expedition-traversal-spec.md). Карточка и статусы принадлежат vault. Этот audit не закрывает A1 и не описывает native authoring Game Studio как готовую возможность.

## Реализация

Квалифицированный `LayeredCollisionWorld` использует конечные 3D boxes и axis-aligned ramp slabs. Feet Y — максимум поверхности под полным квадратным footprint, с намеренным воздушным зазором у нижней кромки наклона. Локально соединённые поверхности сохраняют непрерывную высоту через стыки; swept полный объём проверяет бок/низ/голову. Ни highest-floor snap, ни колонн под мостом нет. Внешний float-contact tolerance не закрывает внутренние отверстия.

Project schema 1 связывает две scene schema 3: named spawns, конечную геометрию, лестницы, portals, recovery threshold и группы представления. Загрузчик проверяет все ссылки до входа и снова проверяет относительный Content path/reparse points при disk reload кандидата. Spawn/recovery обязаны находиться выше recovery threshold. Старые scene schema 1/2 и C++ save/replay не поддерживаются.

`ExpeditionSession` владеет одним fixed clock, Explore/Paused и безопасной точкой текущей сцены. Portal и ladder выбираются одним фильтрованным контекстом перед distance/id. FSM дополнен FallingStanding/FallingCrouched; падение сохраняет принятый объём. Capture лестницы проходит supported путь, включая crest; дальнейшие проверенные коридоры сохраняют ограниченную интерполяцию. Пауза/focus flush не передаёт действие resume в мир.

`PartyTrail` записывает фактические fixed-tick/path breakpoints, а не только позиции render frames. Стойка endpoint описывает входящий сегмент и не перезаписывается при stationary crouch/stand. Белый лидер и два tint-спутника имеют общий atlas, отдельные providers и не добавляют коллизии. Дистанции .7/3.2 units позволяют реально показать нижнюю пару и заднего героя наверху после срыва с моста высотой 1.6; память ограничена последними 3.7 units плюс один сегмент. Дальнего спутника могут закрывать независимые объекты; одновременная видимость всей партии не обещается.

Представление строит конечную рампу с восемью геометрическими углами, 24 вершинами для нормалей и согласованным со Stride winding. Occlusion получает параллельные near/far unproject лучи до глубины ног/центра/головы лидера, проверяет настоящие поверхности, защищает опорный deck и распространяет скрытие только по валидированному `HideWith`. Верхний спутник скрывается локально на вырезанной опоре; первый нижний остаётся видимым. Возврат после .15 с отсчитывается по session ticks. World sprite depth сохранён; только HUD SpriteBatch отключает depth test, иначе пол перекрывал текст.

Renderer candidate строит SceneInstance/entities/materials/buffers до Activate. Slot камеры общий с compositor; место для retired bundle резервируется заранее. Activate меняет ссылки, после Advance прежний SceneInstance и generated vertex/index buffers освобождаются. Shared atlas/font живут до закрытия игры. Diagnostic `portal-failure` отбрасывает уже подготовленного кандидата и оставляет прежний мир с читаемой ошибкой. Постоянное число buffers активного bundle проверяется в переходах; это вместе с code review ownership, а не измерение GPU allocator или доказательство отсутствия любых утечек Stride.

## Проверенные срезы и найденные исправления

- `2334561` / `d7b3b2e`: ramp qualification; nominal float-edge coverage исправлен после воспроизведения, внутренние отверстия сохранены.
- `6c63baf` / `551c002`: Core/session; исправлены unsafe spawn ниже recovery threshold, доступный контекст на наклоне и повторная проверка Content reparse path после старта.
- `928ce0e`: подход к лестнице через crest следует supported пути вместо проникающей хорды; per-tick clearance/support/speed в обоих направлениях.
- `02c56e5` / `a214c19`: trail/occlusion; stationary stance больше не переписывает пройденный crouched сегмент под потолком. Независимые Core-ревью одобрены.
- `fc28e6c`: Windows/session integration, GUI renderer, command routes, verifier и текущие companion distances. До коммита Core 62/62 и Windows build прошли; реальный GPU preview выявил и исправил winding рампы и HUD depth.
- `29424d5` / `1695529`: анимация лидера тоже останавливается на паузе. Проверка удерживает паузу 12 updates, пересекающих animation interval. Реальная mutation без guard дала sprite indices 7→6 при frames 158→170 и ticks 612→612; восстановленный guard дал 7→7. Артефакты `build/stride-game/p13-pause-missing-guard-repro/` и `p13-pause-guard-verified/`. Мутация восстановлена в `finally`, не коммитилась и не вошла в финальный пакет. Оба runtime-ревью и узкая перепроверка `1695529` одобрены.

Первые автоматизированные previews лежат в `build/stride-game/p13-*-preview*`. Они подтвердили native D3D11/RTX 4090, ramp ascent/descent, верх/низ, локальный cut, пустую арку/дальнюю стену, смешанные ярусы спутников, 20 portal legs и паузу на лестнице. Ведущий визуально проверил соответствующие PNG. Эти предварительные артефакты не заменяют итоговый извлечённый ZIP.

## Сборка и итоговая проверка

`scripts/stride/build-game.ps1` успешно собрал финальный `build/stride-game/20260909-062511-258/rat-expedition-0.1.0-win-x64.zip`. Manifest HEAD — `12c648d533a7d93b5c99f01660ed40b3f5e5cad0`, меняющий только карточку после runtime `16955296c7cc0b42ae6db07a24f57641301726d6`; `gameWorkingTreeDirty=false`. SHA256 ZIP: `3FDB1B97513E50EE704E554193708D57A14F625D943578CBF2F07E23DAFC8D4B`. Upstream `e2c786a45f69917bf233793f6a097b150e2fe264`, SDK 10.0.300, packages 4.4.0-dev, self-contained win-x64, local effect compiler. Core — 62 passed / 0 failed. Publish выдал один CS0162 в generated temporary build code; ошибок нет. ZIP содержит runtime, shader database/native DLL, обе сцены/project, PNG, Noto/OFL, credits и dependency notices.

Предварительный пакет `20260909-061233-647` из `fc28e6c` прошёл все 29 сценариев: `C:/5_gamedev/rat-expedition-validation/20260909-061309-191/verification.json`. Он предшествовал последнему pause animation regression. Пакет `20260909-062046-591` был промежуточным и заменён до финальной проверки. Оба не являются итоговым артефактом этой записи.

`scripts/stride/verify-game.ps1` из `1695529` успешно завершил **29/29 сценариев, exit 0**. Итог: `C:/5_gamedev/rat-expedition-validation/20260909-062548-975/verification.json`; SHA256 в результате совпадает с финальным ZIP. Приложение извлечено в `extracted/`, working directory — отдельный `unrelated-working-directory/`. Это 16 успешных GPU/пакетных сценариев и 13 ожидаемых стартовых отказов с exit 1/читаемым `error.log`.

Verifier сохраняет прежние 17 executable scenarios и добавляет layered720/1080, mixed heights, 20 portal legs, renderer rejection, три recovery fixtures и четыре metadata negatives. Старый camera-edge fixture получает конечные бортики, поскольку обычный P1.3 теперь допускает падение; отдельный upper-void fixture доказывает восстановление верхней safe point без пола снизу. Large-Y проверяет только bounded startup декоративной лестницы, не крупномасштабную точность мира.

Ключевые итоговые доказательства ниже относительно `C:/5_gamedev/rat-expedition-validation/20260909-062548-975/`:

| Артефакты | Наблюдение |
| --- | --- |
| `layered-1280x720/session-ramp-ascent.png`, `session-bridge-lower.png` | Конечная рампа видима под ногами; нижний cut локален. |
| `layered-1920x1080/session-bridge-upper.png`, `session-bridge-lower.png`, соседний `run.json` | Настоящие 1920×1080; в одинаковых XZ feetY 1.6/0, deck защищён сверху и скрыт снизу. |
| `layered-1280x720/session-arch-empty.png`, `session-upper-rail.png`, `session-offcentre-behind-wall.png` | Пустая арка не скрывается; rail скрывается независимо от опорного deck; северная стена за глубиной героя остаётся. |
| `mixed-companions/session-mixed-companions.png`, `run.json` | Высоты лидер/спутники 0/0/1.6, visibility true/true/false; hidden только bridge-cut/bridge-rail-cut. |
| `body-1280x720/` и `body-1920x1080/`: `session-ladder-pause-start.png`, `session-ladder-paused.png`, `run.json` | Frames 158→170, ticks 612→612, Paused, sprite index 7→7 в обоих разрешениях; HUD читаем. |
| `portal-roundtrips/run.json`, `session-portal-01.png` … `session-portal-20.png` | 20 legs / WorldRevision 20, reset истории/групп, стабильное число buffers активной сцены. |
| `renderer-candidate-failure/session-candidate-rejected.png`, `run.json` | Отказ уже подготовленного renderer candidate, прежний courtyard и revision 0 сохранены. |
| `recovery-courtyard/`, `recovery-sluice/`, `recovery-upper-void/`: `session-recovered.png`, `run.json` | Standing / revision 1, восстановлены уровни 0/0/1.6. В reset партия совпадает на safe point; это не три отдельно видимых силуэта. |

Ведущий отдельно осмотрел итоговые 720p ramp/lower/paused, native 1080p upper/lower, mixed и upper-void recovery PNG. Это визуальная проверка конкретных финальных кадров, не непрерывного движения или ручного управления. После прогона игровые процессы закрыты; дополнительные runtime изменения не внесены.

Промежуточный полный Release Game Studio после принятия Core/session: exit 0, 69.23 с, пять upstream NU5100, ноль ошибок; `C:/5_gamedev/stride/logs/rat-foundation/20260909-054649-025/result.json`. Upstream checkout чист. Текущий repository Python suite: 22 tests passed (запуск ведущего). C++ runtime retired; прежние CTest не используются как доказательство этой игры.

Финальный полный Release Game Studio пересобран ведущим после runtime-среза: exit 0, 72.12 с, пять upstream NU5100, ноль ошибок. Evidence: `C:/5_gamedev/stride/logs/rat-foundation/20260909-062643-742/result.json`; editor `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`, версия `4.4.0-dev`. Upstream после сборки чист; финальный repository Python rerun — 22 tests / 5.688 с / pass. Эта сборка не утверждает, что текущие code-first карты уже открываются как native editor scenes.

## Пределы доказательств

### Оставшаяся направленная ручная проверка

Открыть `Rat.Expedition.Windows.exe` из финального извлечённого пакета. Белая крыса — лидер; голубая и оранжевая следуют за ним. Начинать с обычного запуска без `--smoke-*`. Для второго размера можно добавить `--width 1920 --height 1080`. Результат этой проверки пока **не записан как выполненный**; полный [P1](../rat-expedition-traversal-spec.md) и его зависимость A1 не закрываются только автоматическими тестами.

| Действие и ориентир | Что проверить руками |
| --- | --- |
| WASD в стартовом дворе, колесо мыши | Движение относительно экрана понятно; крупность 4.5–7 удобна, лидер/подсказка читаемы; нет заметного дрожания, скачков камеры или неприемлемого мерцания PNG при движении. |
| Низкая перекладина с двумя столбиками слева и выше стартовой позиции | Стоя проход закрыт. Удерживать Ctrl, войти под перекладину и отпустить: остаться пригнувшимся с «Здесь нельзя встать». После полного выхода встать; оценить читаемость крысы под геометрией. |
| Коричневая лестница за низким проходом, к отдельной верхней площадке | У входа новое E/Enter, затем W/S в обе стороны. Ctrl не ломает подъём; выход сохраняет высоту ног. Проверить, что ступеньки не рисуются поверх лица и удержанный E не захватывает лестницу повторно после выхода. |
| Дальний участок за центральной стеной: широкая рампа и длинный мост | Обойти стену, подняться по рампе и спуститься обратно. Затем пройти под мостом с земли. Сверху настил под ногами остаётся; снизу локальный вырез открывает лидера, не убирая дальние стены. Оценить понятность высоты и поведения спутников. |
| Голубая метка справа и ниже стартовой позиции | По подсказке новое E/Enter переводит в водосброс. Там обратная метка возвращает во двор. После появления на новой площадке нет мгновенного возврата от удержанного Enter. Десять циклов уже покрывает автоматический маршрут; вручную важна понятность перехода. |
| Esc при движении, у метки и на лестнице; Alt+Tab и возврат | Позиция и анимация замерли, подсказка сообщает паузу. После возврата фокуса пауза сохраняется; Esc/Enter продолжает. Enter продолжения не активирует портал/лестницу; новое действие работает после отпускания. Колесо после возврата не даёт скачок zoom. |

Закрыть окно кнопкой X. Если проверка выявит дефект, записать сцену/ориентир, размер окна, точную последовательность клавиш и ожидаемое/фактическое поведение; не выдавать наблюдение кадров ниже за замену этой проверки.

GPU routes подают обычные команды и снимают настоящий backbuffer; runtime position setters для маршрутов отсутствуют. Focus callbacks и wheel deltas синтетически вызывают действующий input path, не системные события мыши/клавиатуры. Извлечённый ZIP запускается из другого cwd на текущем development PC; отдельной чистой машины без SDK не было. Направленная визуальная проверка кадров не равна полному ручному плейтесту, оценке движения на всех дисплеях или remote CI. Native Game Studio import/edit/save карт и всех ресурсов остаётся следующим A1, без начала его реализации.
