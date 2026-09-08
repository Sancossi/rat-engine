---
type: task
area: Game
status: Not started
task_type: Feature
sprint:
due:
tags: [task, expedition]
---

# Прототип: путь крысы и первая Windows-сцена

Intent: Игрок проходит 3D-двор 2D-героем через обычный путь, низкий лаз и подъём.

Specification: [[GDD]] — Исследование; [[rat-expedition-art-audio]] — камера/перекрытия; [[ADR-017 Rat expedition uses rat-engine]]; [P1: интерфейсы, срезы, сценарии](../../../docs/rat-expedition-traversal-spec.md); [архитектура](../../../docs/rat-expedition-architecture.md).

Dependencies: [[expedition-engine-decision]]. До закрытия зависимостей не брать в работу.

## Acceptance

- Собственный rat-engine воспроизводимо собирает отдельный rat-game через game-release preset и Windows ZIP без editor UI; записаны версия, команда/настройки и путь артефакта.
- WASD, приседание, контекстный подъём и безопасные выходы сверху/снизу работают по A4; спутники не блокируют проход.
- Временный спрайт различим перед/за стеной и на высоте; проверены 1280×720 и 1920×1080, кадры/видео приложены.
- Есть две связанные серые площадки двор/водосброс, устойчивый обратный переход и безопасная точка восстановления при срыве.
- Пауза блокирует движение; действие не протекает из меню в мир; направленный ручной тест и проверки коллизий/перехода записаны.

## Границы

Только временные ассеты, движение и контекст. Без универсального редактора, свободной камеры и полного MGS-стелса.

Origin: [[expedition-engine-decision]]; [[ADR-017 Rat expedition uses rat-engine]]; [[design-rat-expedition-preproduction]]; [[production/roadmap/rat-expedition-release-roadmap|План до релиза]].
Follow-up: [[expedition-prototype-interactions]].

## Resolution

Ещё не выполнялась. При закрытии записать поведение, сборку/команды, результаты и ограничения проверки.

## Bugs found

Ещё не проверено. При выполнении записать найденные баги со ссылками или none.
