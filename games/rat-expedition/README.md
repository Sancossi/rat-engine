# Rat Expedition — прототип перемещения на Stride

Самостоятельный Windows x64 прототип: ортографический двор, временный PNG герой,
WASD относительно экрана, низкий лаз, лестница, конечная рампа и мост над нижним путём.
Удерживайте Ctrl для приседания; под потолком герой не встаёт до полного выхода.
Новое E/Enter у лестницы начинает подъём/спуск, W/S двигают по ней в обе стороны.
У края можно упасть; падение ниже порога возвращает последнюю безопасную опору
текущей сцены. Два tint-спутника повторяют полный 3D путь лидера без коллайдеров.
E/Enter у голубой метки переводит между двором и водосбросом. Esc ставит паузу;
Esc/Enter продолжает игру. Потеря фокуса оставляет игру на паузе до явного продолжения.
Повторное контекстное действие требует отпускания клавиши.

Из корня репозитория после подготовки закреплённого Stride:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/build-game.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/verify-game.ps1 -PackageZip <путь-к-полученному-ZIP>
```

Build создаёт отдельный timestamp-каталог `build/stride-game/` с publish, ZIP,
manifest и логами, сохраняя предыдущие результаты. Запустить извлечённый
`Rat.Expedition.Windows.exe`; Game Studio для игры не требуется. Окно закрывается
обычной кнопкой X. `--width 1920 --height 1080` выбирают второй проверяемый размер.

Камера следует за героем без вращения. Колесо мыши приближает/отдаляет сцену:
OrthographicSize 4.5–7, по умолчанию 5.0. `--camera-size 4.5` задаёт начальный zoom.
У краёв карты камера останавливает свой центр, сохраняя героя в кадре.
Диагностика `--smoke-route edges --smoke-frames 1440` обходит края пола;
verifier проверяет кадрирование на отдельном fixture с бортиками, а падение —
на обычных площадках и верхнем настиле над пустотой.
`--smoke-route zoom --smoke-frames 360` проверяет пределы колеса и сброс после focus
callbacks. Это синтетические команды в реальном приложении, не системные события мыши.

`--smoke-route body --smoke-frames 1200` проходит лаз, проверяет отказ вставания,
поднимается на площадку, ходит по ней и спускается. Сохраняются именованные
`body-*.png` и FSM milestones. Маршрут подаёт обычные команды; координаты не подменяются.
Русские подсказки загружаются из поставляемого Noto Sans с OFL.
Карты находятся в `Rat.Expedition.Authoring/Assets`: `ExpeditionProject.sdscene`
связывает `Courtyard` и `Sluice`. Native components задают solids/ramps, spawns,
лестницы, portals и occlusion; `HideWith` связывает детали выреза. Native Entity.Id
служит игровым id, DefaultSpawn — явная ссылка. Вид и Core получают геометрию из
одного authored источника. Старый JSON остаётся только историческим Core fixture
и не входит в новый ZIP; runtime не использует его как fallback.

`--smoke-route layered --smoke-frames 1800` проверяет рампу в обе стороны,
одинаковые XZ сверху/снизу, арку, перила и дальнюю стену; `mixed` — спутников на
разных ярусах; `portals` — десять круговых переходов. `recovery` использует обычное
падение, `portal-failure` диагностически отклоняет уже подготовленного renderer
кандидата и проверяет сохранение старой сцены. Именованные `session-*.png` и
telemetry фиксируют фактические снимки/группы. Карты редактируются в Game Studio
по [контракту A1.2](../../docs/stride-native-map-authoring-spec.md); библиотека
моделей, звука, UI и native sprite sheet остаётся следующим срезом A1.3.

`--smoke-frames 360 --evidence-dir <каталог>` выполняет фиксированный ввод по тому
же motor, снимает настоящий backbuffer до стены/перед ней/за ней и завершает игру.
Это автоматизированная проверка, не ручной плейтест. `--content-dir` нужен для
изолированных проверок отказов PNG/шрифта; по умолчанию Content читается рядом с exe,
независимо от working directory. Стартовые ошибки контента завершаются ненулевым
кодом и `error.log`; отказ кандидата перехода сохраняет текущую сцену и выводит подсказку.
Обычные диагностические записи расположены в `%LOCALAPPDATA%/RatExpedition/logs`.

Smoke (`--smoke-frames > 0`) ограничен одинаковыми 60 Hz в активном, неактивном,
скрытом и свёрнутом окне. Vsync отключён; при hidden/minimized пропускается только
desktop Present, сам draw/backbuffer capture выполняется. Значения elapsed маршрутов
и физика не меняются: обычные walls/zoom используют 1/60, прежние длинные маршруты
1/30 на update. Это одинаковый темп автоматизации между состояниями окна, не обещание
60 FPS на любом GPU или реального времени для каждого диагностического маршрута.

`verify-game.ps1` включает четыре коротких `window-states` прогона. Helper
`scripts/stride/verify-smoke-window-states.ps1` активирует своё окно на несколько
секунд, затем отдельными запусками проверяет unfocused, hidden и настоящий minimize
через Windows API. В `run.json/smokeTiming` записываются Stopwatch после загрузки
контента, кадры/draws/ticks, root HWND, OS foreground/visibility/IsIconic и backend.
Сравниваются темп, конечная позиция/ticks и разные непустые GPU-кадры.
`smoke-progress.json` служит готовностью для внешнего helper; старые каталоги
доказательств не переиспользуются. Синтетическая pause/focus/wheel проверка отдельно
выполняет прежние callbacks, она не заменяет наблюдение OS-состояния.

Политика квалифицирована на закреплённом Windows backend: реальный запуск использует
DesktopWinForms, его focus/minimize не меняет GameBase.IsActive (upstream callbacks
не подключены). Обычный запуск и обработчики pause/flush/resume этим исправлением
не меняются; новый ручной Alt-Tab тест обычной игры не заявлен. Другой backend,
который действительно сбрасывает IsActive, требует отдельной квалификации.

## Native карты и квалификация

В Game Studio открыть `Rat.Expedition.sln`, затем asset `Courtyard` или `Sluice`.
У `Expedition geometry` менять `Size`, у Transform — `Position`; для текущей
физики Rotation должна быть identity, Scale — `(1,1,1)`, включая родителей.
Размеры задают одновременно editor preview и runtime collision. Ошибки содержат
asset/entity/property. Компилировать и заново запускать игру после Save; hot reload
открытого database bundle не обещается. [MCP](../../tools/stride-mcp/README.md)
позволяет inspect/edit/undo/redo/save по native ids без ввода мышью/клавиатурой.

Обычный `scripts/stride/build-game.ps1` упаковывает production assets. Для полного
executable verifier построить пакет с `-IncludeQualificationAssets`, затем вызвать
`scripts/stride/verify-game.ps1 -PackageZip <ZIP>`. Эта опция добавляет отдельные
native QA assets, производные от authored карт, через тот же Stride compiler.
`--native-project QA/<fixture>` доступен только при `--smoke-frames > 0`;
обычный запуск всегда загружает `ExpeditionProject`. Никакого JSON override нет.
`native-candidate-failure` подаёт реальному переходу отдельно скомпилированный
невалидный native candidate; active scene и presentation должны сохраниться.
`native-world.json` — выходной диагностический экспорт, не авторский файл.

## Работа в Zed

Откройте каталог `games/rat-expedition` как папку проекта Zed. C# language server
может загрузить `Rat.Expedition.slnx` с игровыми проектами и ссылками Stride.
Если окно уже было открыто до добавления решения, выполните в палитре команд
`editor: restart language server` из открытого C# файла.

## Проверка и зависимости

Core scenarios запускаются без GPU:

```powershell
dotnet run --project games/rat-expedition/Rat.Expedition.Core.Tests -c Release
```

Зависимости закреплены exact package versions и `packages.lock.json`. First-party
Stride поступает из local feed исходной основы; `Stride.Dependencies.*` и внешние
зависимости — из NuGet по lock. Cache `.packages` изолирован на игру. Publish содержит
runtime, shader database, native DLL, credits и доступные license notices/inventory.
Сборка сама не очищает caches, не обновляет движок и не публикует релиз в сети.
