---
type: adr
area: Engine
status: Accepted
decided: 2026-08-31
tags: [adr]
notion_id: 3ccf3827-36cc-817f-b266-fa8b6786683c
---

# ADR-007 Events and maps stored as JSON

## Context

Нужен data-driven формат для карт/событий с edit-in-playmode и git.

## Decision

Authoring: **JSON**. UX команд: RM-like list. Binary/SQLite — не MVP; binary cache можно добавить позже для shipping.

## Consequences

- Schema + validation обязательны.
- Hot-reload/hot-apply проще.
- Diff в PR читаемый.
