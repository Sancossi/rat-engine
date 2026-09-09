# P1.1a — крупность и следование полевой камеры

Замечание пользователя: крыса слишком мала, нужен более близкий полевой кадр с читаемым соотношением героя и препятствий. Пользователь уточнил ориентир: Final Fantasy / Chrono Trigger. Это качественное направление пользователя; точное соответствие чужой сцене не заявляется, внешние изображения в игру не добавлены. Числовые параметры остаются проверяемым предложением A7 в [art note](../../vault/game/expedition/rat-expedition-art-audio.md).

[Контракт P1.1a](../stride-first-game-slice-spec.md) реализован в `bbd7f3d317607ffe9018471674f6826a6915f7ee`: OrthographicSize 14 → 5.0, колесо 4.5–7; фиксированные yaw 45°/pitch 39.5212°, следование с aim +.6 по Y и clamp центра до 1.5 units от floor edge. Для узкого пола inset ограничен половиной его размера. Upright sprite сохраняет ширину .533 units и depth; scaleY 1.2963625 компенсирует вертикальное сокращение проекцией. Геометрия, PNG, motor, Core и upstream Stride не изменены.

## Визуальный результат

Первый preview 5.5 дал примерно 80 px видимого героя при 720p; выбран 5.0 с примерно 90 px, около 12.5% высоты кадра. Это оценка видимых пикселей, а не размер прозрачного quad. Старый кадр P1.1 давал примерно 25 px. Пропорции PNG читаются лучше, герой перед стеной остаётся целым, а за стеной перекрывается. Родительский агент просмотрел preview 5.0 и подтвердил читаемость; это не полная художественная приёмка A7.

Дополнительное read-only измерение frame 30: bbox сиреневых пикселей героя (`B > R+3` и `R > G+3`, отличает их от серо-зелёного окружения) даёт прежние 25 px / 3.47% при 720p, новые 92 px / 12.78% при 720p и 137 px / 12.69% при 1080p. Это воспроизводимая цветовая оценка данного placeholder, не универсальный анализ силуэтов. Полный прозрачный quad в projection telemetry занимает 16% высоты на default zoom.

Сравнение на диске: прежний `build/stride-game/20260909-025311-964/` и его validation `C:/5_gamedev/rat-expedition-validation/20260909-025340-312/gpu-1280x720/frame-0030.png`; новые preview — `build/stride-game/camera-preview-5/frame-0030.png`, `frame-0180.png`, `frame-0360.png`. Финальные кадры перечислены ниже; preview не является финальным ZIP.

## Воспроизведение

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/build-game.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/verify-game.ps1 -PackageZip build/stride-game/20260909-031619-404/rat-expedition-0.1.0-win-x64.zip
```

Финальная сборка выполнена после фиксации кода; [manifest](../../build/stride-game/20260909-031619-404/publish/build-manifest.json) содержит game HEAD `bbd7f3d317607ffe9018471674f6826a6915f7ee`, `gameWorkingTreeDirty=false` для игровых путей, upstream `e2c786a45f69917bf233793f6a097b150e2fe264`, локальные пакеты Stride 4.4.0-dev и SDK 10.0.300. Постороннее dirty дерево сохранено; source checkout Stride чистый.

- [ZIP](../../build/stride-game/20260909-031619-404/rat-expedition-0.1.0-win-x64.zip): 72 108 274 bytes; SHA256 `E973F8B804236BDA4A739DFEFBD0068C3C673540ADFB3DC0FF710C89F8331538`.
- [Exe](../../build/stride-game/20260909-031619-404/publish/Rat.Expedition.Windows.exe), [build result](../../build/stride-game/20260909-031619-404/result.json), [12 Core scenarios](../../build/stride-game/20260909-031619-404/core-tests.log), [publish log](../../build/stride-game/20260909-031619-404/publish.log).
- Предварительная проверка того же кода до commit: `C:/5_gamedev/rat-expedition-validation/20260909-031345-080/verification.json`, все 10 executable сценариев passed.
- Первый повтор финального ZIP: `C:/5_gamedev/rat-expedition-validation/20260909-031648-450/` воспроизвёл слишком короткий timeout edge-сценария; этот неполный прогон не заявляется как passed. ZIP извлекается в `extracted/` вне checkout, cwd — `unrelated-working-directory/`.
- Итоговый прогон использует verification-скрипт commit `4c6bab3aa2faa6539900c18478bb0b4ee35dabb0` и каталог `C:/5_gamedev/rat-expedition-validation/20260909-032117-947/`: `verification.json` подтверждает все 10 сценариев, команда завершилась с exit 0. Пять успешных GPU запусков дали exit 0, пять ожидаемых PNG/JSON отказов — exit 1 и читаемые error logs. Default кадры лежат в `gpu-1280x720/` и `gpu-1920x1080/`; edge кадры — `camera-edges-4.5/` и `camera-edges-7/`; каждый GPU сценарий сохраняет `run.json`.

## Проверяемое поведение

12 существующих Core сценариев прошли без новых тестов на константы камеры. Реальная проверка приложения включает default 1280×720 и 1920×1080; обход всех четырёх краёв на zoom 4.5 и 7; wheel/focus маршрут; пять прежних отрицательных PNG/JSON сценариев. Default и edges сохраняют реальные GPU PNG и `run.json` с adapter, размером, настройками камеры и экранными bounds полного 32×48 quad, включая прозрачные поля. Это консервативный bounds, а не измерение видимой площади крысы или проверка её перекрытия.

Итоговый edge маршрут достиг X [-7.3, 7.8], Z [-5.3, 5.8]. При самом близком zoom 4.5 полный quad остался внутри экрана: суммарный X .281–.719, Y .284–.881. Frame 420 показывает ближайший угол пола; 720/1080/1440 сохраняют другие части обхода. Камера не вращается и не меняет направления WASD. Колесо проходит через тот же clamp/discard путь в приложении: большие положительные/отрицательные дельты упираются в 4.5/7, после focus callbacks устаревшая дельта игнорируется, обычная дельта возвращает размер 5.0.

## Границы проверки

В review и повторном запуске обнаружен дефект harness: 1440 кадров не помещаются в прежние 30 секунд вместе со startup/capture. Промежуточный запуск с 60 секундами также воспроизвёл timeout. Фактический pinned `Stride.Games/GameBase.cs:84` задаёт `MinimizedMinimumUpdateRate = new ThreadThrottler(15)`; unfocused/hidden окно требует около 96 секунд на 1440 кадров. Поэтому промежуточные 90 секунд (`e066304`) заменены ограниченными 150 секундами в `4c6bab3`; обычные сценарии сохраняют 30 секунд. Заменённый 90-секундный прогон остановлен по известному PID wrapper и его единственному child с точным validation executable path. Runtime ZIP не менялся и не перепаковывался ради внешнего verification-скрипта. Два независимых review одобрили результат: функциональный/edge review — `bbd7f3d`, blind review после исправления timeout — `4c6bab3`. Статусами карточки управляет родительский агент.

Родительский агент выполнил итоговые сборки редакторов: Stride GameStudio Release exit 0, 78.82 с, пять upstream NU5100 warnings, 0 errors; лог `C:/5_gamedev/stride/logs/rat-foundation/20260909-031718-286/`, executable `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. Source checkout остался чистым. Legacy `scripts/verify.ps1` exit 0, 24 Python tests, 720 CTest passed + один skip из 721; лог `C:/Users/bogor/AppData/Local/Temp/stride-camera-final-legacy.log`, executable `C:/5_gamedev/rat-engine/build/dev-release/apps/editor/rat-editor.exe`. Это проверка editor/source основы и сохранности старой реализации, а не дополнительные gameplay-тесты P1.1a.

Проверка выполняется на Windows ПК разработчика с реальным NVIDIA GeForce RTX 4090 / Direct3D11 и self-contained runtime 10.0.8. Это синтетические команды и прямые focus callbacks в реальном GPU приложении; ручной wheel/Alt-Tab плейтест и запуск на чистой машине не заявляются. Нормальные окна автоматически закрываются после bounded route; timeout останавливает только созданный этим сценарием процесс. Publish сохраняет прежние shader/native/runtime/content/credits, локальный effect compiler и выключенный remote compiler. Остался прежний upstream CS0162 warning, без новых ошибок сборки. P1.2/P1.3, pixel-stable A7 и новые механики этим срезом не закрываются.
