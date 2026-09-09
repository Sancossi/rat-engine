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

Preview не заменяет committed ZIP, полный verifier и финальную Release-сборку Game Studio.
Эти артефакты и независимое ревью записывает ведущий при закрытии карточки.
Ручной gameplay/Alt-Tab, чистая машина и remote CI этим исправлением не заявлены.
