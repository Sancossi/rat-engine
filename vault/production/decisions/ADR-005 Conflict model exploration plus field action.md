---
type: adr
area: Game
status: Accepted
decided: 2026-08-31
tags: [adr]
notion_id: 3ccf3827-36cc-8177-b43f-f6560bd60c14
---

# ADR-005 Conflict model exploration plus field action

## Context

Конфликт/бой в духе продукта.

## Decision

MVP: **exploration + quests/dialogue/puzzles** + **light field action on map**. Нет отдельного turn-based battle screen.

## Consequences

- Нет Battle Processing в Event v1.
- Field action = interact/collision/simple skills на карте.
- Deep RPG combat progression — out of scope MVP.
