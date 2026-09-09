---
type: bug
area: Game
status: Investigating
review: In review
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

Locked Stride `GameBase` задаёт `MinimizedMinimumUpdateRate=15`, применяемый также к скрытому/нефокусному окну. `DrawWhileMinimized=false` может остановить счётчик smoke-кадров в `EndDraw`. `Tick` отдельно возвращается при `IsActive=false`; простого изменения лимита недостаточно без проверки реальных событий окна. Маршруты используют фиксированное elapsed на каждый update, поэтому 15 Hz замедляют их по настенным часам.

Specification: [P1.3](../../../docs/stride-session-layered-traversal-spec.md); [game README](../../../games/rat-expedition/README.md); locked `Stride.Games/GameBase.cs`, `Stride.Core/ThreadThrottler.cs`.

## Acceptance

- Given `--smoke-frames > 0`, when окно активно/неактивно/скрыто/свёрнуто, then отсутствует специальный 15 Hz лимит и пропуск обязательных draw; задан одинаковый ограниченный темп (предпочтительно 60 Hz), а не неограниченная загрузка CPU/GPU.
- Given настоящий Windows minimize/focus change, when smoke идёт в фоне, then маршрут завершается, пишет ожидаемые непустые GPU-кадры, прогресс/ticks и измеренное время/состояние окна. Проверка не выдаёт вызовы callbacks за OS-события.
- Given обычная игра без smoke, when теряется фокус и затем возвращается, then прежняя pause/flush/resume семантика сохраняется. Синтетические focus/zoom/ladder pause регрессии остаются содержательными; тестовый режим не поглощает их незаметно.
- Параметры времени маршрутов и Core physics не ускоряются ради результата; реальный темп измерен отдельно от startup/asset compile и состояния presenter. Не обещать строгий FPS на любом GPU.
- Новый committed Release ZIP проходит Core/executable checks; добавлена направленная регрессия фонового/свёрнутого запуска, README и verifier отражают режим. Независимое read-only ревью, vault и полная Release-сборка редактора записаны.

Origin: [[expedition-prototype-traversal]].

## Resolution

Исследование/исправление только режима автоматизированного прогона. A1 и новая графика не входят в задачу.

## Bugs found

Не проверено — работа начата.
