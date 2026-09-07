---
type: adr
area: Engine
status: Accepted
decided: 2026-09-04
tags: [adr]
---

# ADR-014 Play executes event graphs

## Context

[[research-event-node-graph|research: Event node graph vs bytecode]] (Sprint 8) хранил граф только как authoring: Play исполнял `commands[]`, Edit компилировал graph → list. Canvas вырос до всех `CommandOp`, но ноды — подписи, а карты (grey_yard) по-прежнему пишутся списком команд.

Авторы хотят UX как у визуального event editor (поля **внутри** ноды) и **полный** отказ от command-list: и Edit, и Play живут на графе.

## Decision

**Источник истины page-body — `graph`.** `EventRuntime` шагает по нодам и рёбрам (`entry` → kind → `exit`). Поле `commands[]` снимается со схемы и runtime.

Лимиты [[ADR-008 Parallel and Autorun runtime limits]] считаются в **нодах за кадр**, не в командах. Yield (Wait, Set Move Route) — на текущей ноде, как сейчас на команде.

Старые карты только с `commands`: loader **один раз** поднимает линейный/вложенный список в граф (`commands_to_graph`), дальше save пишет только `graph`.

Trigger и page `conditions` остаются вне графа (вкладки Event Graph).

Не интерпретируем Ruby/JS. Не второй VM рядом с command-stepper: stepper заменяется graph-walker.

## Consequences

- Нужна миграция JSON + тесты runtime на graph yield/branch.
- Reverse-compile нужен хотя бы как loader-path, иначе grey_yard сломается.
- Compile `graph → commands` уходит из apply/save.
- UX: виджеты на ноде, не отдельный command list.

Origin: chat 2026-09-04. Related: [[research-event-node-graph|research: Event node graph vs bytecode]], [[Event System]], [[feat-play-walks-event-graph|feat: Play walks event graph]].
