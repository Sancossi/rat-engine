---
title: "Техническое сравнение открытых движков для Rat Expedition"
type: note
research_type: technical
topic: "Открытая основа игрового движка для Rat Expedition"
decision: "Стоит ли заменить rat-engine до продолжения P1"
source: native-run
status: complete
preset: standard
validation: normal
created: 2026-09-08
updated: 2026-09-08
claims_verified: 15
claims_unverified: 8
claims_disputed: 2
claims_overturned: 1
tags: [research, expedition, engine]
---

# Техническое сравнение открытых движков для Rat Expedition

**Последующее решение пользователя, 2026-09-08:** выбран Stride — [[ADR-018 Rat expedition uses Stride]]. Ниже сохранены результаты сравнения и первоначальная рекомендация как история исследования. Текущее направление интеграции описано в [записке о Stride и MCP](../../../../docs/stride-mcp-integration-research.md); рекомендация Godot spike ниже больше не является следующим действием проекта.

**Решение, которому служит исследование:** стоит ли заменить rat-engine до продолжения P1.

## Резюме

**Рекомендация: не переносить игру сразу, а провести один ограниченный spike на Godot 4.7.2.** По результатам сравнения документации Godot получил наивысшую оценку. Он прямо поддерживает анимированные 2D-спрайты в 3D, ортографическую камеру, готовый editor/export pipeline и разрешительную MIT-лицензию. Stride — второй кандидат, если C#/.NET и engine-in-loop тесты окажутся важнее зрелости экосистемы. [1][2][3][5][6][8][11]

По лицензиям Godot, Stride и Wicked почти не различаются; выбор определяют готовый путь 2D-in-3D и ежедневные затраты на authoring. O3DE превосходит остальные варианты по тестовой инфраструктуре, но его ресурсные требования сводят это преимущество на нет для небольшого проекта. Rat-engine остаётся конкурентоспособным благодаря уже выполненной работе: это ценный актив и одновременно источник будущих расходов на инструменты редактора и игры. [6][17][24][25]

Ни один внешний кандидат не тестировался на нашей сцене. При переходе на Godot придётся отказаться от части работающего C++ P1.1 и заменить существующие headless regressions новым harness. Решение о миграции следует принимать только после одного экспортируемого spike.

## Взвешенная матрица

Веса отражают приоритеты Rat Expedition: скорость/authoring 25%, соответствие P1 20%, лицензия/контроль 15%, низкая цена миграции 15%, тесты/Windows 15%, зрелость 10%. Шкала 1–5: 1 — требование не закрыто или требует смены важного ограничения; 3 — решаемо заметной прикладной работой; 5 — поддерживается напрямую и доказано. Итог равен сумме `оценка / 5 × вес`. Это аналитическая оценка документации и текущего проекта, а не результат одинаковых прототипов.

| Основа | Скорость/authoring 25% | P1 fit 20% | Лицензия/контроль 15% | Низкая цена миграции 15% | Тесты/Windows 15% | Зрелость 10% | Итог |
| --- | ---: | ---: | ---: | ---: | ---: | ---: | ---: |
| **Godot 4.7.2** | 5 | 5 | 5 | 2 | 4 | 5 | **88** |
| **Stride 4.3** | 4 | 4 | 5 | 2 | 4 | 3 | **75** |
| rat-engine, контроль | 2 | 4 | 5 | 5 | 5 | 1 | **73** |
| **Wicked Engine** | 2 | 3 | 5 | 3 | 3 | 3 | **61** |
| **O3DE** | 2 | 3 | 4 | 1 | 5 | 4 | **60** |
| **id Tech 4 fork** | 1 | 2 | 2 | 1 | 2 | 2 | **32** |

Все кандидаты закрывают базовые требования Windows, offline и доступности исходного кода. Если закрытый исходный код игры станет обязательным, id Tech 4 не пройдёт hard gate из-за GPL. Если обязательным станет C++, Godot и Stride потеряют баллы миграции, а Wicked станет главным challenger, хотя его production workflow всё ещё требует проверки.

## Рекомендации

1. Собрать **один Godot 4.7.2 spike**: 3D-сцена, анимированный billboard hero, ортографический target 640×360, wall depth и локальный occluder, стоя/присев через низкий проход, ramp/bridge, headless rule test и Windows ZIP вне checkout.
2. Оставить исходные JSON/PNG и игровые правила независимыми от движка. Не переносить старые editor caches и сохранить ветку rat-engine.
3. Снять сопоставимые данные: время реализации, clean build и packaging, размеры workspace и ZIP, RAM редактора, cold asset import, кадры 1280×720 и 1920×1080, pixel stability, occlusion, test command и найденные ограничения.
4. Если spike проходит P1-критерии и authoring заметно проще, подготовить новый ADR, явно заменяющий ADR-017. Если нет, закрыть spike и разблокировать текущий P1 на rat-engine.
5. Проверять Stride только при провале Godot по тестируемости, C# integration или pixel/occlusion behavior; отдельно подтвердить xUnit и self-contained `win-x64` на 4.3/4.4. Проверять Wicked только если C++ станет hard gate. O3DE и id Tech 4 не прототипировать без нового ограничения, меняющего матрицу.
6. Перед выпуском проверить точный состав third-party notices выбранной версии.

## Соответствие P1 и authoring

Документация **Godot** прямо описывает `Sprite3D`/`AnimatedSprite3D` в 3D-мире, billboard/depth/shading/shadows и orthogonal camera. `CharacterBody3D` и scene workflow дают основу для реализации перемещения; crouch, ladder и локальное скрытие перекрытий остаются нашим игровым кодом. [1][2][3]

**Stride** также имеет сильный native fit: SpriteComponent занимает 3D-пространство, `Billboard` ориентирует спрайт на камеру, depth ignore опционален, sprite-sheet animation поддерживается. Bepu CharacterComponent можно расширять на C#. Динамическая высота тела, лестница и occluder policy требуют проверки в spike; документация не доказывает, что их можно реализовать без изменений движка. [8][9]

**Wicked Engine** предоставляет editor, Lua, C++, WISCENE и character samples. Найдены смежные sprite primitives, но не подтверждён готовый editor-authored depth-tested animated character component. Это кандидат для engine-level разработки, а не доказанный короткий путь к P1. [13]

**O3DE** имеет camera-facing SubUV particle sprites. Они относятся к VFX и по умолчанию не взаимодействуют с depth buffer без изменения shader, поэтому не заменяют готовый компонент персонажа. Наличие нужного depth-tested character pipeline всё ещё не подтверждено. [18]

**id Tech 4** силён в brush-based 3D-картах, lighting, collision и entity scripting, но официальные и community-документы не показывают поддержанный orthographic animated-billboard character workflow. Для нашей игры придётся адаптировать FPS-ориентированную архитектуру.

## Поставка, тестирование и цена перехода

Godot CLI запускает сцены и скрипты в headless-режиме, поддерживает fixed FPS и экспорт Windows; текущий GUT 9.7.1 даёт CLI test runner для Godot 4.7.x. Это сторонний harness, а не продолжение существующих CTest. [4][7]

Stride документирует Windows x64 publish и self-contained deployment. Официальный xUnit пример способен запустить Game с fixed timestep и проверять scene behavior, но независимо проверенная страница документации относится к версии 4.2. Это преимущество нужно подтвердить на выбранной версии 4.3/4.4. [10][11]

Wicked имеет Visual Studio/CMake, static library, Editor, Windows template и Samples/Tests. Не найден документированный headless gameplay assertion harness; утверждение, что затраты на его эксплуатацию обязательно выше Stride, остаётся непроверенным.

O3DE предлагает самый глубокий automation stack: CTest, GoogleTest, PyTest, editor Python, crash monitoring и artifacts. Цена — документированные 16–32 GB RAM, около 40 GB свободного дискового пространства для установки через installer или 100+ GB для исходников в зависимости от конфигурации проекта, плюс Asset Processor, Gems и tool targets. [16][17]

У id Tech 4 Win64 зависит от выбранного community fork. GPL, отсутствие свободных Doom assets и раздельные editor/runtime проекты повышают exit cost. Это разумная основа для GPL PC-FPS, но слабое совпадение с малой 2.5D RPG.

Текущий rat-engine уже собирает отдельный `rat-game` и Windows ZIP. Проходят 720 тестов, один пропускается из-за ограничений прав. Движок содержит fixed-tick traversal, surfaces, ramps, ladders, JSON и CTest. Любая миграция должна повторно доказать эти свойства. [24][25][26]

## Противоречащие данные

- Stride может оказаться лучше Godot для разработки с упором на C# и автоматизацию: официальный пример запускает движок внутри xUnit-теста. Его совместимость с 4.3/4.4 пока не подтверждена.
- Wicked имеет editor, Lua, CI и tests; формулировка «это только renderer/framework» слишком сильна. Не доказан именно нужный gameplay/sprite workflow.
- O3DE имеет stock billboard/SubUV renderer, поэтому отсутствие 2.5D primitives опровергнуто. Наличие нужного depth-tested character pipeline всё ещё не подтверждено.
- id Tech 4 имеет community-форки Win64; проект не заброшен. Его последнее место отражает несовпадение с проектом и GPL/интеграционную цену, а не техническую невозможность.

## Ландшафт, лицензии и зрелость

| Основа | Лицензия и выпуск | Оценка зрелости и риск |
| --- | --- | --- |
| **Godot** | MIT; стабильная 4.7.2 от 2026-08-18; коммерческая игра может иметь собственную лицензию при сохранении уведомлений. [5][6] | Самая зрелая экосистема из сравниваемых специализированных open-source движков; Windows и единый 2D/3D workflow. |
| **Stride** | MIT; стабильная 4.3.0.2507 и 4.4.0-beta6. [12][27] | Экосистема меньше Godot. Точный состав third-party notices нужно проверить для выбранной сборки. |
| **Wicked Engine** | MIT C++ engine/editor; v0.72.113 от 2026-08-24. [13][14] | Windows pipeline и исходники открыты. Сопровождение небольшим числом людей представляет риск, однако формальная оценка bus factor не проводилась; вывод имеет низкую уверенность. |
| **O3DE** | Выбор Apache-2.0/MIT и отдельные условия bundled components; последний найденный официальный релиз — 26.05.0. [15][28] | Сведения о версии вышли за установленное месячное окно актуальности. Техническая база сильна, но масштаб повышает цену solo-разработки. |
| **id Tech 4** | Исторический Doom 3 BFG GPL-3.0 source drop без retail game data и части интеграций. Коммерческое распространение разрешено, но получатели производного executable получают права GPL и Corresponding Source. [19][20] | Поддержка Win64 зависит от community-форка; актуальные пути дают dhewm3 и RBDOOM, mapping — DarkRadiant. [21][22][23] |

## Источники

| № | Поддерживаемый вывод | Издатель | Дата | Проверено | Уверенность |
| --- | --- | --- | --- | --- | --- |
| [1] | 2D/3D и orthogonal camera | [Godot Engine](https://docs.godotengine.org/en/4.7/tutorials/3d/introduction_to_3d.html) | n.d. | 2026-09-08 | высокая |
| [2] | SpriteBase3D billboard/depth/shadows | [Godot Engine](https://docs.godotengine.org/en/4.7/classes/class_spritebase3d.html) | n.d. | 2026-09-08 | высокая |
| [3] | CharacterBody3D и scene workflow | [Godot Engine](https://docs.godotengine.org/en/latest/getting_started/first_3d_game/03.player_movement_code.html) | n.d. | 2026-09-08 | высокая |
| [4] | Headless CLI и Windows export | [Godot Engine](https://docs.godotengine.org/en/stable/tutorials/editor/command_line_tutorial.html) | n.d. | 2026-09-08 | высокая |
| [5] | Godot 4.7.2 release | [Godot Engine](https://godotengine.org/download/archive/4.7.2-stable/) | 2026-08-18 | 2026-09-08 | высокая |
| [6] | MIT и proprietary game compliance | [Godot contributors](https://github.com/godotengine/godot-docs/blob/master/about/complying_with_licenses.rst) | n.d. | 2026-09-08 | высокая |
| [7] | GUT 9.7.1 CLI для Godot 4.7 | [GUT project](https://github.com/bitwes/Gut) | n.d. | 2026-09-08 | высокая |
| [8] | Stride world-space billboard sprites | [Stride project](https://doc.stride3d.net/4.3/en/manual/sprites/use-sprites.html) | n.d. | 2026-09-08 | высокая |
| [9] | Stride Bepu CharacterComponent | [Stride project](https://doc.stride3d.net/latest/en/manual/physics/characters.html) | n.d. | 2026-09-08 | высокая |
| [10] | Stride Windows/self-contained publish | [Stride project](https://doc.stride3d.net/4.2/en/manual/files-and-folders/distribute-a-game.html) | n.d. | 2026-09-08 | средняя |
| [11] | Stride engine-in-loop xUnit pattern | [Stride project](https://doc.stride3d.net/4.2/en/manual/troubleshooting/unit-tests.html) | n.d. | 2026-09-08 | средняя, актуальность не подтверждена |
| [12] | Stride release lines и MIT package | [NuGet Gallery](https://packages.nuget.org/packages/Stride.Core.Assets/4.4.0-beta6) | 2026-09-08 | 2026-09-08 | высокая |
| [13] | Wicked features/build/editor | [Wicked Engine](https://github.com/turanszkij/WickedEngine) | n.d. | 2026-09-08 | высокая |
| [14] | Wicked v0.72.113 | [Wicked Engine](https://github.com/turanszkij/WickedEngine/releases) | 2026-08-24 | 2026-09-08 | высокая |
| [15] | O3DE dual license и components | [O3DE](https://github.com/o3de/o3de/blob/development/LICENSE.txt) | n.d. | 2026-09-08 | высокая |
| [16] | O3DE Windows footprint/toolchain | [O3DE](https://docs.o3de.org/docs/welcome-guide/requirements/) | n.d. | 2026-09-08 | высокая |
| [17] | O3DE automated testing | [O3DE](https://docs.o3de.org/docs/user-guide/testing/getting-started/) | n.d. | 2026-09-08 | высокая |
| [18] | O3DE camera-facing SubUV particles | [O3DE](https://docs.o3de.org/docs/user-guide/visualization/particles/particle-editor/module-renderer/) | n.d. | 2026-09-08 | высокая |
| [19] | Doom 3 BFG exclusions и source drop | [id Software](https://github.com/id-Software/DOOM-3-BFG/blob/master/README.txt) | 2012 | 2026-09-08 | высокая |
| [20] | GPLv3 distribution obligations | [Free Software Foundation](https://www.gnu.org/licenses/gpl-3.0.en.html) | 2007-06-29 | 2026-09-08 | высокая |
| [21] | dhewm3 x64 build и maintenance | [dhewm3 project](https://github.com/dhewm/dhewm3/releases/tag/1.5.5) | 2026-06-08 | 2026-09-08 | высокая |
| [22] | RBDOOM Win64 modernization | [RBDOOM project](https://github.com/RobertBeckebans/RBDOOM-3-BFG/releases) | 2025-05-10 | 2026-09-08 | высокая |
| [23] | Current idTech4 mapping tools | [DarkRadiant](https://www.darkradiant.net/) | n.d. | 2026-09-08 | высокая |
| [24] | Работающий P1.1 и результаты тестов | [Rat Expedition project](../../../../docs/archive/cpp/audits/2026-09-08-rat-expedition-traversal.md) | 2026-09-08 | 2026-09-08 | высокая |
| [25] | Текущая архитектурная база | [Rat Expedition project](../../../../docs/rat-expedition-architecture.md) | 2026-09-08 | 2026-09-08 | высокая |
| [26] | Цена перехода и прежняя оценка | [Rat Expedition project](../../../../docs/rat-expedition-engine-comparison.md) | 2026-09-08 | 2026-09-08 | высокая |
| [27] | MIT-лицензия Stride engine/editor | [Stride project](https://github.com/stride3d/stride/blob/master/LICENSE.md) | n.d. | 2026-09-08 | высокая |
| [28] | O3DE 26.05.0 release line | [O3DE](https://www.docs.o3de.org/docs/release-notes/) | 2026-05-27 | 2026-09-08 | высокая |

## Карта устаревания

| Утверждение | Класс | Перепроверить |
| --- | --- | --- |
| Godot 4.7.2 — current stable | version | 2026-09-18 |
| Godot 2D-in-3D/export compatibility | compatibility | 2026-10-08 |
| Stride 4.3/4.4 — current lines | version | 2026-10-08 |
| Stride billboard compatibility | compatibility | 2026-10-08 |
| Wicked v0.72.113 — current release | version | 2026-09-24 |
| O3DE 26.05.0 — latest documented release | version | **просрочено с 2026-06-27; проверить перед действием** |
| dhewm3 1.5.5 ecosystem signal | ecosystem | 2026-12-08 |
| Лицензии и bundled notices | license | 2027-09-08 |

Ближайшая плановая перепроверка назначена на 2026-09-18. Сведения о версии O3DE уже вышли за месячное окно актуальности и требуют обновления перед выбором этого кандидата.
