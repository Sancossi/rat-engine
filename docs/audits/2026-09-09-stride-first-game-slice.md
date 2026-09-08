# P1.1 Stride — сборка и проверка первой сцены

Проверен только [срез P1.1](../stride-first-game-slice-spec.md): отдельная Windows игра, ортографический серый двор, оригинальный PNG герой, camera-relative WASD, границы пола и стены. Полный [P1](../rat-expedition-traversal-spec.md), включая лаз, лестницу, мост, спутников и переходы, остаётся за пределами этой реализации. MCP не добавлялся. Статусы и дальнейшая очередь находятся только в vault.

Основа планирования — commit `3862666`. Реализация: `bef318311508d42c174a36a8cd5303bb80cc48ca`; исправления после review: `d02c5bded9beda0efdccfcc81742dbf8fb6fc4df`. Два независимых reviewer одобрили исправленную реализацию; завершение карточки и спринта ведёт родительский агент. Финальная упаковка выполнена после фиксации кода, при HEAD `8d55216609eade62b848d0f49b958b2aa089a5fe` (следующий commit содержит только передачу на review). Manifest показывает `gameWorkingTreeDirty=false` для проверяемых игровых путей; это не утверждение о чистоте всего репозитория. Постороннее dirty C++ дерево сохранено.

## Воспроизведение и артефакты

Из корня репозитория выполнены:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/build-game.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/verify-game.ps1 -PackageZip build/stride-game/20260909-025311-964/rat-expedition-0.1.0-win-x64.zip
```

Обе команды завершились с кодом 0. Финальные артефакты:

- [ZIP](../../build/stride-game/20260909-025311-964/rat-expedition-0.1.0-win-x64.zip), 72 103 707 bytes; SHA256 `CD941C7721E14CCD2095E0C95C5822C17865BFE9FF4C79B4375BBE542E32E368`.
- [Executable](../../build/stride-game/20260909-025311-964/publish/Rat.Expedition.Windows.exe), [manifest](../../build/stride-game/20260909-025311-964/publish/build-manifest.json), [build result](../../build/stride-game/20260909-025311-964/result.json), [Core results](../../build/stride-game/20260909-025311-964/core-tests.log), [publish log](../../build/stride-game/20260909-025311-964/publish.log).
- Изолированная проверка: `C:/5_gamedev/rat-expedition-validation/20260909-025340-312/verification.json`. ZIP извлечён в соседний `extracted/`, рабочий каталог процесса — `unrelated-working-directory/`, оба вне checkout.
- Реальные кадры: подкаталоги `gpu-1280x720/` и `gpu-1920x1080/` указанного validation-каталога, каждый содержит `run.json` и `frame-0030.png`, `frame-0180.png`, `frame-0360.png`.

Закреплённый upstream `e2c786a45f69917bf233793f6a097b150e2fe264` остался чистым. Использованы локальные пакеты Stride `4.4.0-dev`, SDK `10.0.300`, self-contained runtime `10.0.8`, Windows x64 и Direct3D11 на NVIDIA GeForce RTX 4090. Package locks и source mapping исключают stable fallback для first-party Stride; внешние зависимости восстановлены по lock. Publish содержит shader database `data/`, native DLL, runtime, PNG/JSON, credits, лицензии и inventory зависимостей. Remote effect compiler явно выключен в runtime configuration. Финальный publish прошёл с одним upstream code-generation warning CS0162; исправление движка не требовалось.

## Результаты и исправления

Все 12 Core сценариев прошли: экранные направления и скорость, нормировка диагонали, тонкая стена и скольжение, края пола, ограниченный catch-up, сброс дробного tick при потере фокуса, ошибочные spawn/bounds, нечисловые и переполняющие диапазоны, повторные ids/поднятая стена, некорректный JSON и отсутствующие обязательные координаты. Проверяются наблюдаемые позиции и ошибки, без графических зависимостей.

Все 7 сценариев реального извлечённого executable прошли. Два GPU запуска по 360 кадров завершились с кодом 0 и фактическими backbuffer размерами 1280×720 и 1920×1080. Пять отрицательных случаев — отсутствующий PNG, повреждённый PNG, отсутствующий JSON, повреждённый JSON и отсутствующий `spawn.x` — завершились с кодом 1 и читаемым `error.log`, без зависшего окна. Ошибка асинхронного LoadContent передаётся в итоговый exit code.

Кадр 180 показывает целого героя перед стеной; кадр 360 — корректное перекрытие стеной после обхода её края. Первоначальный полный camera billboard наклонял верх sprite в стену: upright sprite с yaw 45°, PixelsPerUnit 60 и обычной depth-проверкой устранил пересечение. Alpha PNG сохраняется согласованно с `PremultipliedAlpha=false`. PNG и его процедурный источник скопированы в игровые каталоги; повторная генерация даёт те же байты, внешние изображения не использовались. Загрузка default settings первоначально перезаписывала запрос 1920×1080; явные code-first settings сохраняют заданный размер, который теперь проверяет verifier.

Review обнаружил, что отсутствующая вложенная координата JSON могла превратиться в 0. Включён `RespectRequiredConstructorParameters`; regressions исключают X/Y/Z в spawn и границах, отдельный executable-сценарий исключает `spawn.x`. Midpoint коробок вычисляется как `min + (max - min) / 2`, без переполнения суммы конечных координат.

Stride пропускает Update у неактивного окна, поэтому сброс только внутри Update был недостаточен. Реальные OnDeactivated/OnActivated callbacks сбрасывают motor accumulator и elapsed time; первый ввод после возвращения фокуса отбрасывается. В обоих GPU прогонах `focusProbePassed=true`: диагностика напрямую вызывает эти callbacks и проверяет отсутствие добавленного движения/ticks. Это проверка callback-пути, а не инъекция системного события фокуса.

## Границы доказательства

Родительский агент отдельно выполнил итоговый legacy `scripts/verify.ps1`: exit 0, 24 Python tests, 720 CTest passed и один symlink skip из 721. Старый Release editor сохранён по пути `C:/5_gamedev/rat-engine/build/dev-release/apps/editor/rat-editor.exe`; лог — `C:/Users/bogor/AppData/Local/Temp/stride-p11-final-legacy.log`. Это проверка сохранности исторической основы, а не тесты игровой сцены Stride.

Это реальный GPU render и запуск полного ZIP с другим cwd на данном ПК разработчика. Автоматический маршрут использует тот же motor, но не заменяет ручной WASD плейтест. Процессы запускались скрыто и завершились; окна игры не оставлены открытыми. Чистая машина без SDK/editor не использовалась, сетевой доступ на уровне ОС не блокировался. Self-contained/runtime/content closure проверена, однако такая проверка не доказывает независимый запуск на любой Windows машине. Многоуровневая физика, pixel-stable 640×360 и остальные требования полного P1 здесь не заявлены.
