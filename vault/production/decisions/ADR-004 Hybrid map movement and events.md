---
type: adr
area: Game
status: Accepted
decided: 2026-08-31
tags: [adr]
notion_id: 3ccf3827-36cc-816b-a99b-f56d2964fea6
---

# ADR-004 Hybrid map movement and events

## Context

Нужна модель карты под RM-like events и field action.

## Decision

**Гибрид:** игрок ходит свободно; авторы снапит к сетке; события вешаются на **tile** и/или **trigger volume**.

## Consequences

- Event runtime должен резолвить оба типа триггеров.
- Edit gizmos: grid + volume.
- Чистый tile-step movement не обязателен.
