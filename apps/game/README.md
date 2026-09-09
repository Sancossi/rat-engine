# Rat Expedition: P1.1

Историческая C++ реализация от 2026-09-08, сохранённая при переходе на Stride.
Текущая игра и её сборка: [games/rat-expedition](../../games/rat-expedition/README.md).
Нижеследующие команды и ограничения относятся только к прежнему прототипу;
его исходный контракт — [архив P1](../../docs/rat-expedition-traversal-spec-rat-engine-2026-09-08.md).

Самостоятельное приложение использует `rat_core`/`rat_engine`, без панелей редактора.
Сейчас доступен серый двор, WASD относительно фиксированной камеры 3/4, дисковый
PNG крысы, пауза Escape и русские подсказки. Приседание, лестница, мост, спутники и
переходы относятся к следующим срезам одной карточки P1.

Сборка из корня репозитория на Windows:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/verify.ps1 -Preset game-release
build/game-release/apps/game/rat-game.exe
```

Либо в Developer PowerShell с CMake/Ninja:

```powershell
cmake --preset game-release
cmake --build --preset game-release
ctest --preset game-release
cpack --config build/game-release/CPackConfig.cmake -B build/game-release
```

Пакет: `build/game-release/rat-expedition-0.1.0-Windows-x64.zip`. В нём executable,
runtime DLL, `data/expedition`, существующий Noto Sans/OFL и лицензии зависимостей.
Входные данные по умолчанию разрешаются от executable, а не working directory.
`--data-dir` меняет только корень экспедиции; шрифт остаётся рядом с executable.
Игра не читает и не создаёт пользовательские сохранения.

Диагностический прогон через обычный renderer:

```powershell
build/game-release/apps/game/rat-game.exe --hidden --frames 120 --width 1920 --height 1080 --screenshot build/game.png --report build/game.json
```

`--frames` задаёт конечный прогон с двумя фиксированными simulation ticks на кадр;
в обычной игре accumulator использует wall time. Это удобно для неподвижных
снимков, но не заменяет ручную проверку движения/фокуса. `--hidden` создаёт скрытое
настоящее окно и использует GPU; `--renderer software-d3d11` выбирается только явно.
Report содержит фактический backend/vendor/device, позицию, число кадров/ticks,
разрешения и CPU frame timing (не GPU time и не обещание FPS). Screenshot ожидает
завершения callback до освобождения GPU. Ошибка входных данных/PNG/шрифта или записи
диагностики приводит к exit 1 с описанием в stderr.

Графика мира рисуется в 640×360, затем nearest integer fit с letterbox; UI рисуется
в разрешении окна. Окно не уменьшается ниже 640×360. Sprite feet anchor снапится
только при отображении; физические координаты сохраняют точность. Временный atlas
32×48 × 2 кадра × 4 направления пересоздаётся командой:

```powershell
python assets/expedition/rat_sprite.py
```

Это оригинальная программно нарисованная пиксельная заготовка; пользовательские
референсы не перерабатывались. Исходник/палитра и provenance: `assets/expedition/`.
Для greybox-преград с настоящими боковыми гранями используется существующая
occupancy-геометрия; старые blocker markers редактора остаются прежними.

Project/scene schema 1 сейчас принимает пустые `portals`, `low_passages` и
`occluder_groups`, явно отвергая ещё не реализованные записи. Карты сохраняют
существующий schema 5 и строгую проверку duplicate keys. Новые метаданные тоже
отклоняют duplicate keys, неверные id, нечисловые координаты, недоступный spawn и
пути за пределы каталога данных (включая разрешённые символьные ссылки).
