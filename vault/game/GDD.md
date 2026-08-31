---
type: note
tags: [game, gdd]
---

# GDD

> **Status:** Working decisions (2026-08-31). Согласовано в чате. Open questions — внизу.
> Референс духа: RPG Maker + edit-in-playmode. Бой: field action light, не turn-based screen.

## One-liner

Исследуй стилизованный 3D-пиксельный мир, решай квесты через события и лёгкий action на карте — и правь карту/ивенты, не выходя из playmode.

## Pillars

1. **Author while playing** — увидел проблему → правил карту/событие → сразу проверил.
2. **Event-driven story** — switches / variables / pages / triggers в духе RPG Maker.
3. **Readable 3D pixel** — ortho 3/4, пиксель-перфект без ряби и субпиксельного дрожания.

Каждая фича должна усиливать хотя бы один pillar.

## Core loop

1. **Motivator** — квест / загадка / NPC / блокеры на карте
2. **Action** — ходить, говорить, триггерить события; при нужде — короткий field action
3. **Reward** — прогресс истории, новый путь, предмет, смена switches
4. **Progression hook** — новое событие открывает зону / ветку; в playmode можно донастроить контент

## Fantasy / тон

- **Рабочее имя:** Rat Project
- Жанр: exploration adventure + light field action
- **Сеттинг:** средневековье **без магии** + лёгкий **стимпанк** (пар, механизмы, латунь/шестерни — без заклинаний)
- Тон: приземлённый, «рукотворный» прогресс вместо магии

## Механики

### Primary

- Исследование карты (hybrid movement)
- Диалоги и ветвления через [[Event System]]
- Квесты / загадки на switches & variables
- Edit-in-playmode: карта + события

### Secondary / support

- Field action: столкновения, простые скиллы/интеракты на карте (без battle scene MVP)
- **Инвентарь:** модель ближе к **RPG Maker** (список предметов, stack/qty, key items; без сложных сетки-слотов как в Diablo)
- Save / load состояния switches/variables + позиция + инвентарь

### Explicit non-goals (MVP)

- Классический turn-based battle screen
- Полноценная combat-RPG прогрессия (уровни/экип deep)
- Мультиплеер

## Карта и движение

- **Модель:** гибрид — свободное движение игрока; дизайн снапится к сетке — [[ADR-004 Hybrid map movement and events]]
- **События:** можно вешать на **клетку** и на **объём-зону** (trigger volume)
- **Камера:** orthographic **3/4** — [[ADR-003 Ortho pixel-stable camera]]
- Коллизии: пропы + невидимые blockers; кубы террейна (клетка height-grid) и заборы на рёбрах клеток (`edge_barriers`); сетка помогает авторам, не диктует «клетка за клеткой» геймплей

## Event System (RM-inspired)

Минимальный набор для MVP:

- **Triggers:** Action button, Player touch, Event touch, Autorun, Parallel (осторожно)
- **Pages** + условия (switches / variables / items / self-switch)
- **Commands (v1):** Show Text, Control Switches/Variables, Conditional Branch, Set Move Route (базовый), Transfer Player, Change Items, Wait, Comment
- Данные: data-driven (не хардкод квестов в C++)

Детали — [[Event System]].

## Edit-in-playmode

**Цель vertical slice:** править и карту, и события в том же билде, что и игра. [[ADR-006 Edit-in-playmode author loop]]

**Порядок внедрения:**

1. События на «серой» карте
2. Правка карты (тайлы/пропы/коллайдеры) в playmode
3. Полный author-loop (карта + события вместе)

Режимы: Play / Edit (hotkey), без обязательного отдельного «редакторского exe» на MVP.

## Прогрессия

- Short session (5–15 мин): один квест или загадка на маленькой карте
- Mid: цепочка из 3–5 событий / комнат
- Long: кампания TBD после vertical slice

## Контент и vertical slice

- 1 небольшая карта (интерьер или двор)
- 2–3 NPC + 1 puzzle/quest на switches
- 1 пример field action (например отпугнуть/сломать блокер)
- Демонстрация edit-in-playmode (переставить проп + поправить event page)

## UX / UI

- HUD: минимум (interact prompt)
- Диалоговое окно (RM-like)
- Edit overlay: гизмо/сетка/inspector события (ImGui) — мышь во вьюпорте: [[feat: Mouse viewport map edit]]
- Pause / save

## Риски

- Design: расползание scope «ещё одна RM-команда»
- Tech: pixel-perfect ortho + движущаяся камера (shimmer)
- Tech: hot-reload карты/событий без десинхрона
- Prod: edit-in-playmode UX сложнее, чем оффлайн-редактор

## Закрытые вопросы

- Название (рабочее): **Rat Project**
- Сеттинг: **средневековье без магии + лёгкий стимпанк**
- Texel / scale: **base 32px**, integer upscale под PC
- Инвентарь: **RM-like** (list + qty / key items)
- Персонажи: **3D mesh + процедурные анимации**, иерархия сегментов как в классическом **Resident Evil** (голова, грудь, живот, руки из нескольких сегментов, ноги сегментами) — [[ADR-009 RE-like segmented character hierarchy]]
- Crafting / economy в MVP: **нет**
- Platform: **Windows first**

## Open questions

- [x] Формат хранения events → **JSON** ([[ADR-007 Events and maps stored as JSON]])
- [x] Parallel limits → **8 active / 32 cmds/frame / no nested** ([[ADR-008 Parallel and Autorun runtime limits]])
- [ ] Ночь/день — нужна ли в MVP
- [ ] Конкретные арт-референсы (игры/артисты)
