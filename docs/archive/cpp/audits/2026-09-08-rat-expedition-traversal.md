# Rat Expedition — evidence реализации P1

Исторический отчёт C++ реализации от 2026-09-08, сохранённый без переноса его
приёмки на Stride. Точные прежние контракты: [архитектура](../rat-expedition-architecture-rat-engine-2026-09-08.md)
и [P1](../rat-expedition-traversal-spec-rat-engine-2026-09-08.md). Текущая Stride
реализация проверена отдельно в [P1.1](../../../audits/2026-09-09-stride-first-game-slice.md)
и [P1.1a](../../../audits/2026-09-09-stride-camera-framing.md).

Карточка: [P1](../../../../vault/production/tasks/expedition-prototype-traversal.md).
Контракт: [traversal specification](../rat-expedition-traversal-spec-rat-engine-2026-09-08.md).

## P1.1: самостоятельный запуск, PNG и игровая сборка

Исполнитель применил `gds-dev-story` с проектной адаптацией: утверждённая карточка
вместо новой story, только выделенный P1.1, статусы/галочки у ведущего агента,
независимое ревью до следующего среза. Это не заявление о завершении всего P1.

Реализованы `apps/game/rat-game`, `game-release`, выбор frontend dependencies без
miniaudio/editor UI, общие window/input/process/ImGui-bgfx helpers в `apps/platform`
с совместимыми editor headers. `rat_core` и его форматы не менялись. Переносимые
Project/Scene/ExpeditionSession и отдельный `rat_expedition_tests` связывают только
ядро/JSON/Catch2. Обе программы используют общий процессный путь/Unicode argv.

В новой карте есть плоский двор и occupancy-преграда. Спрайт загружается с диска,
декодируется закреплённым bimg_decode в RGBA8, рисуется billboard с cutout/depth,
nearest и feet anchor. Мир 640×360 увеличивается целочисленно; Noto Sans загружается
с диска для native-resolution UI. GPU resources освобождаются до bgfx shutdown.
Камера фиксирована 3/4; прыжок/rotation/interact в этом срезе не подключены.

Проверки рабочего дерева P1.1:

- `scripts/verify.ps1`: полный dev-release редактор; CTest 721 зарегистрирован,
  **720 прошли, 1 skipped**, failures 0. Пропущен только symlink escape test:
  текущему Windows-процессу не разрешено создание symlink. Link-isolation прошёл.
  Python: 16 прошли. Log: `build/p1-1-dev-verify.log`.
- `cmake --preset game-release`, build и CTest: тот же результат 720 passed / 1
  skipped. На этой машине для новой binary directory переданы
  `FETCHCONTENT_SOURCE_DIR_*` на уже скачанные pinned sources из dev-release;
  версий/зависимостей это не меняет. Log: `build/p1-1-game-verify.log`, точная
  локальная команда сохранена в `build/p1-1-game-build.ps1`.
- Headless проверки нового target: стабильный spawn/idle, детерминированное
  camera-relative movement, отказ path traversal/missing PNG, malformed schema,
  blocked/unsupported spawn, duplicate keys project/map, числовой CLI.
- Реальный Auto renderer, скрытое GLFW-окно: `Direct3D 11`, vendor `4318` (`0x10de`),
  device `9860` (`0x2684`). `--frames 30 --hidden --screenshot ... --report ...`
  завершился exit 0; `build/p1-1-game-1280.png`, `build/p1-1-game-report.json`.
  Это реальный отрисованный кадр, не ручной интерактивный playtest.
- ZIP извлечён вне checkout в
  `C:/Users/bogor/AppData/Local/Temp/rat-p1-package-jcqaayrb/`; запуск с working
  directory `%TEMP%`, 1920×1080, Auto renderer, exit 0. PNG/report:
  `build/p1-1-package/package1080.png`, `build/p1-1-package/package.json`.
  В извлечённой копии удаление PNG и malformed scene JSON дали ожидаемый exit 1
  с именем ресурса; после проверки исходные байты восстановлены. Логи ошибок:
  `build/p1-1-package/missing-sprite.txt`, `malformed-metadata.txt`.

Editor artifact: `build/dev-release/apps/editor/rat-editor.exe`.
Game artifact: `build/game-release/apps/game/rat-game.exe`.
Package: `build/game-release/rat-expedition-0.1.0-Windows-x64.zip`.

Оставшаяся приёмка P1: стойка/height policy и replay compatibility, лестницы,
другая сцена/rollback/recovery/input gating, мост сверху/снизу, спутники и локальные
перекрытия. В P1.1 действует прежняя высота физического тела 1,6; игровая 0,8/0,4
вводится только P1.2. Нельзя переносить этот результат на общие критерии P1.
