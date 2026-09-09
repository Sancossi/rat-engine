# Rat Expedition — первая сцена Stride

Самостоятельный Windows x64 прототип: ортографический двор, временный PNG герой,
WASD относительно экрана, пол и стены, низкий лаз и лестница на площадку.
Удерживайте Ctrl для приседания; под потолком герой не встаёт до полного выхода.
Новое E/Enter у лестницы начинает подъём/спуск, W/S двигают по ней в обе стороны.
Верхняя площадка сохраняет высоту ног и останавливает движение у края.
Мосты, спутники, переходы и полная пауза остаются следующими срезами [P1](../../docs/rat-expedition-traversal-spec.md).

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
`--smoke-route zoom --smoke-frames 360` проверяет пределы колеса и сброс после focus
callbacks. Это синтетические команды в реальном приложении, не системные события мыши.

`--smoke-route body --smoke-frames 1200` проходит лаз, проверяет отказ вставания,
поднимается на площадку, ходит по ней и спускается. Сохраняются именованные
`body-*.png` и FSM milestones. Маршрут подаёт обычные команды; координаты не подменяются.
Русские подсказки загружаются из поставляемого Noto Sans с OFL.
Scene schema 2 задаёт конечные solid structures и валидированные ladder segments;
она не совместима с прежней schema 1 или legacy C++ save/replay.

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
