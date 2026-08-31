---
type: adr
area: Production
status: Accepted
decided: 2026-08-31
tags: [adr]
notion_id: 3ccf3827-36cc-8143-8cc7-e27aba23abae
---

# ADR-006 Edit-in-playmode author loop

## Context

Edit-in-playmode — ключевой pillar.

## Decision

Цель vertical slice: править **карту и события** в playmode. Порядок поставки: events-on-grey-box → map edit → full author-loop.

## Consequences

- Один exe play/edit (hotkey), не обязательный отдельный editor app на MVP.
- Нужен hot-apply данных карты/events.
