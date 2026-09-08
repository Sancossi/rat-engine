# Rat Expedition — первая сцена Stride

Самостоятельный Windows x64 прототип: ортографический двор, временный PNG герой,
WASD относительно экрана, пол и стены. Герой остаётся перед стеной целым, за ней
перекрывается глубиной. Это P1.1; приседание, мосты, лестницы, спутники и переходы
относятся к следующим срезам [P1](../../docs/rat-expedition-traversal-spec.md).

Из корня репозитория после подготовки закреплённого Stride:

```powershell
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/build-game.ps1
powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/verify-game.ps1 -PackageZip <путь-к-полученному-ZIP>
```

Build создаёт отдельный timestamp-каталог `build/stride-game/` с publish, ZIP,
manifest и логами, сохраняя предыдущие результаты. Запустить извлечённый
`Rat.Expedition.Windows.exe`; Game Studio для игры не требуется. Окно закрывается
обычной кнопкой X. `--width 1920 --height 1080` выбирают второй проверяемый размер.

`--smoke-frames 360 --evidence-dir <каталог>` выполняет фиксированный ввод по тому
же motor, снимает настоящий backbuffer до стены/перед ней/за ней и завершает игру.
Это автоматизированная проверка, не ручной плейтест. `--content-dir` нужен для
изолированных проверок отказов PNG/JSON; по умолчанию Content читается рядом с exe,
независимо от working directory. Ошибки завершаются ненулевым кодом и `error.log`.
Обычные диагностические записи расположены в `%LOCALAPPDATA%/RatExpedition/logs`.

Core scenarios запускаются без GPU:

```powershell
dotnet run --project games/rat-expedition/Rat.Expedition.Core.Tests -c Release
```

Зависимости закреплены exact package versions и `packages.lock.json`. First-party
Stride поступает из local feed исходной основы; `Stride.Dependencies.*` и внешние
зависимости — из NuGet по lock. Cache `.packages` изолирован на игру. Publish содержит
runtime, shader database, native DLL, credits и доступные license notices/inventory.
Сборка сама не очищает caches, не обновляет движок и не публикует релиз в сети.
