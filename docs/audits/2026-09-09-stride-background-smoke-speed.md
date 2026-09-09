# Темп smoke в фоне и при сворачивании

Основание: [замечание пользователя](../../vault/production/bugs/bug-expedition-background-smoke-speed.md).
Изменение ограничено автоматизацией Windows; gameplay, native authoring и upstream не меняются.

## Причина и изменение

Закреплённый `e2c786a45f69917bf233793f6a097b150e2fe264`:
`Stride.Games/GameBase.cs:83–84` задаёт отдельный minimized лимит 15 Hz;
`:554` отключает draw при сворачивании, `:603–609` выбирает лимит по minimized/visible/focus.
`RawTick` может вызвать EndDraw даже без Draw: наличие файла само по себе не доказывает новый кадр.
`Stride.Core/ThreadThrottler.cs:85` предоставляет SetMaxFrequency.
`GameWindowSDL.cs:126–127` и `Desktop/GameWindowWinforms.cs:159–160` не подключают
AppActivated/AppDeactivated; private `GameBase` handlers `:932–944` меняли бы IsActive
до protected callbacks. Реальная проверка обнаружила DesktopWinForms с render-child HWND;
OS-состояние берётся с GetAncestor(GA_ROOT), ownership root проверяется по PID.

Smoke устанавливает 60/60 Hz, DrawWhileMinimized=true, vsync=false; hidden/minimized
не делает desktop Present, но рисует и сохраняет backbuffer. Старые значения elapsed
и количество Core ticks на update сохранены. Внешние callbacks smoke не ставят
диагностическую session на паузу; deliberate synthetic probe явно вызывает прежнюю
pause/flush/resume ветку. Обычная игра не меняется. Квалификация не распространяется
на backend с настоящим IsActive=false: GameBase.Tick тогда возвращается раньше Update.

## Локальная репродукция и preview

- До изменения: `build/stride-game/background-baseline/baseline.json`, существующий
  published executable `20260909-084326-721/publish/`. Между файлами кадров 30→180
  прошло 9.9643 s, около 15.05 Hz; реальный IsIconic=true наблюдался три секунды.
  Это разность file timestamps, не Stopwatch и не доказательство остановки Draw.
- После: `build/stride-game/background-window-preview2/verification.json`:
  focused 59.10, unfocused 59.50, hidden 59.27, minimized 59.43 Hz.
  Stopwatch интервал кадров 60→240 исключает startup/initial shader preparation.
  Во всех режимах draw delta равен frame delta, ticks продолжаются; кадры 60/180
  различны по SHA и содержат сцену. `minimized/frame-0180.png` просмотрен: настоящая
  геометрия, три персонажа и кириллица при OS IsIconic=true; это не desktop screenshot.
- Содержательный контроль состоит из [helper](../../scripts/stride/verify-smoke-window-states.ps1)
  и встроенной [телеметрии](../../games/rat-expedition/Rat.Expedition.Windows/SmokeWindowEvidence.cs).
  Финальный helper дополнительно сравнивает конечные ticks/позицию четырёх запусков.
  Допуски 35–80 Hz и отношение max/min ≤1.5 ловят прежние 15 Hz, сохраняя небольшой
  запас на GPU/захват. При вмешательстве в foreground helper честно отвергает прогон.
- Финальный узкий preview после добавления этих assertions:
  `build/stride-game/background-window-preview4/verification.json`, exit 0;
  focused/unfocused/hidden/minimized = 59.76/59.55/59.40/59.25 Hz.
  Все четыре запуска завершились с 472 ticks и позицией `(3.515264, 0, -0.875825)`.
  Предыдущий preview3 отверг реальную потерю foreground на кадре 120, а не выдал её
  за успех; повторный focused этап выполнялся без параллельной Core-проверки.
  Core: 63 passed / 0 failed; vault и diff checks прошли. Узкая Windows Release
  сборка успешна (одно предупреждение CS0162 во временном upstream build source).

## Итоговая приёмка ведущим

Runtime `17e9410` получил независимое read-only Approved (`stride_blind_review`).
Committed ZIP: `build/stride-game/20260909-090102-491/rat-expedition-0.1.0-win-x64.zip`,
manifest `3cec001024dcbe45ea5c76e6c6229762d4af4097`, gameWorkingTreeDirty=false.
SHA256 `019E4EB50AD38310C9029460528901F14FA71ADB267A473C6B9151A5EE19DC1A` проверен.
Build-game exit 0, 63 Core passed; publish имеет один CS0162 в generated source.

Полный verifier: exit 0, 33/33 (20 успешных + 13 ожидаемых отказов),
`C:/5_gamedev/rat-expedition-validation/20260909-090307-242/verification.json`.
Четыре OS-состояния final ZIP: focused 59.46, unfocused 59.49, hidden 59.60,
minimized 59.37 Hz. Во всех 472 ticks и одинаковая позиция; draw/непустые разные
PNG проверены helper. Ведущий осмотрел финальный `window-states/minimized/frame-0180.png`.
Прежние 29 сценариев сохранены и прошли, включая pause/focus probes и компактную партию.

Полный Release Game Studio: exit 0, 5 NU5100 / 0 ошибок, 4.4.0-dev,
`C:/5_gamedev/stride/logs/rat-foundation/20260909-090146-543/result.json`.
Executable: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`.
Upstream чист. Repository Python: 22 passed; vault/projection/diff checks прошли.
Ручной gameplay/Alt-Tab, чистая машина и remote CI этим исправлением не заявлены.
