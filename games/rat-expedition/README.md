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
Project schema 1 связывает scene schema 3: обязательные named spawns, конечные
solids/ramps, лестницы, portals и локальные occlusion groups. `HideWith` явно
связывает детали выреза с основной группой. JSON — один источник видимой геометрии
и Core-ограничений; scene schema 1/2 и legacy C++ save/replay не поддерживаются.

`--smoke-route layered --smoke-frames 1800` проверяет рампу в обе стороны,
одинаковые XZ сверху/снизу, арку, перила и дальнюю стену; `mixed` — спутников на
разных ярусах; `portals` — десять круговых переходов. `recovery` использует обычное
падение, `portal-failure` диагностически отклоняет уже подготовленного renderer
кандидата и проверяет сохранение старой сцены. Именованные `session-*.png` и
telemetry фиксируют фактические снимки/группы. Это не редактор карт: native
авторинг в Game Studio остаётся [планом A1](../../docs/stride-editor-asset-workflow-plan.md).

`--smoke-frames 360 --evidence-dir <каталог>` выполняет фиксированный ввод по тому
же motor, снимает настоящий backbuffer до стены/перед ней/за ней и завершает игру.
Это автоматизированная проверка, не ручной плейтест. `--content-dir` нужен для
изолированных проверок отказов PNG/JSON; по умолчанию Content читается рядом с exe,
независимо от working directory. Ошибки завершаются ненулевым кодом и `error.log`.
Обычные диагностические записи расположены в `%LOCALAPPDATA%/RatExpedition/logs`.

## Работа в Zed

Откройте каталог `games/rat-expedition` как папку проекта Zed. C# language server
автоматически загрузит `Rat.Expedition.slnx` со всеми тремя проектами и ссылками Stride.
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
