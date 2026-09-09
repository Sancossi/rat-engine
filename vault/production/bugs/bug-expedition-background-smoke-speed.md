---
type: bug
area: Game
status: Fixed
review: Approved
severity: Medium
sprint: Sprint 19
tags: [bug, expedition, testing, windows]
---

# Автотесты замедляются в неактивном и свёрнутом окне

## Repro

1. Запустить штатный smoke-маршрут Windows-приложения.
2. Оставить окно неактивным либо свернуть его.
3. Пользователь 2026-09-09 наблюдает крайне медленное движение и просит одинаковую скорость тестов независимо от фокуса.

## Expected

Автотесты используют одинаковый ограниченный темп обновлений/рисования в активном, неактивном и свёрнутом состоянии, сохраняют маршрут, GPU-снимки и завершение. Обычный запуск без smoke сохраняет игровую паузу при потере фокуса.

## Actual

Locked Stride `GameBase` задаёт `MinimizedMinimumUpdateRate=15`, применяемый также к скрытому/нефокусному окну. `DrawWhileMinimized=false` пропускает Draw; EndDraw при этом может вызываться, поэтому счётчик и наличие PNG не доказывают свежий кадр. `Tick` отдельно возвращается при `IsActive=false`, но текущий DesktopWinForms backend не меняет этот признак при OS focus/minimize. Маршруты используют фиксированное elapsed на каждый update, поэтому 15 Hz замедляют их по настенным часам.

Specification: [P1.3](../../../docs/stride-session-layered-traversal-spec.md); [game README](../../../games/rat-expedition/README.md); locked `Stride.Games/GameBase.cs`, `Stride.Core/ThreadThrottler.cs`.

## Acceptance

- Given `--smoke-frames > 0`, when окно активно/неактивно/скрыто/свёрнуто, then отсутствует специальный 15 Hz лимит и пропуск обязательных draw; задан одинаковый ограниченный темп (предпочтительно 60 Hz), а не неограниченная загрузка CPU/GPU.
- Given настоящий Windows minimize/focus change, when smoke идёт в фоне, then маршрут завершается, пишет ожидаемые непустые GPU-кадры, прогресс/ticks и измеренное время/состояние окна. Проверка не выдаёт вызовы callbacks за OS-события.
- Given обычная игра без smoke, when теряется фокус и затем возвращается, then прежняя pause/flush/resume семантика сохраняется. Синтетические focus/zoom/ladder pause регрессии остаются содержательными; тестовый режим не поглощает их незаметно.
- Параметры времени маршрутов и Core physics не ускоряются ради результата; реальный темп измерен отдельно от startup/asset compile и состояния presenter. Не обещать строгий FPS на любом GPU.
- Новый committed Release ZIP проходит Core/executable checks; добавлена направленная регрессия фонового/свёрнутого запуска, README и verifier отражают режим. Независимое read-only ревью, vault и полная Release-сборка редактора записаны.

Origin: [[expedition-prototype-traversal]].

## Resolution

Исправлено 2026-09-09: runtime `17e941050dec93d2c950e1fedfcd6d41c21a2579`. Smoke автоматически устанавливает общий лимит 60 Hz, рисование при сворачивании и независимый от vsync темп. Скрытое/свёрнутое окно сохраняет backbuffer без desktop Present. Шаги маршрута/Core не изменены; синтетическая focus/pause проверка проходит прежнюю ветку отдельно от внешних событий smoke. Обычный запуск не менялся. Независимое read-only ревью `stride_blind_review`, диапазон `74129c4..17e9410`: Approved; ограничение доказательств — pinned DesktopWinForms.

Итоговый [отчёт](../../../docs/audits/2026-09-09-stride-background-smoke-speed.md) содержит source anchors, baseline/preview и границы. [Новый ZIP](../../../build/stride-game/20260909-090102-491/rat-expedition-0.1.0-win-x64.zip), [exe](../../../build/stride-game/20260909-090102-491/publish/Rat.Expedition.Windows.exe); manifest `3cec001024dcbe45ea5c76e6c6229762d4af4097`, `gameWorkingTreeDirty=false`. SHA256 `019E4EB50AD38310C9029460528901F14FA71ADB267A473C6B9151A5EE19DC1A` совпал с verifier. Build-game exit 0, 63 Core passed, один CS0162 в generated source / 0 ошибок.

Полный verifier exit 0: **33/33** (20 успешных, 13 ожидаемых отказов), `C:/5_gamedev/rat-expedition-validation/20260909-090307-242/verification.json`. Настоящие focused/unfocused/hidden/minimized состояния дали соответственно **59.46/59.49/59.60/59.37 Hz** после startup; во всех четырёх 472 ticks и одинаковая конечная позиция. Проверены ownership корневого HWND, actual OS state, прогресс Draw, разные непустые PNG. Ведущий осмотрел финальный `window-states/minimized/frame-0180.png`. Прежние GPU 720p/1080p, камера, body/ladder pause, мост/спутники, portals/recovery и отказы контента прошли. Ручной плейтест обычной игры/другой backend не заявлен.

Полный Release Game Studio: `scripts/stride/build.ps1`, exit 0, 5 upstream NU5100 / 0 ошибок, `C:/5_gamedev/stride/logs/rat-foundation/20260909-090146-543/result.json`; executable `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. Upstream чист. Python suite: 22 passed; vault/projection/diff checks пройдены при закрытии. A1 и новая графика не начинались.

## Bugs found

none — открытых дефектов исправления не найдено. Оставшаяся ручная приёмка P1 ведётся в исходной карточке.
