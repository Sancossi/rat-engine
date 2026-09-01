---
type: adr
area: Production
status: Accepted
decided: 2026-08-30
tags: [adr]
notion_id: 3ccf3827-36cc-815a-9c82-e5a72851c9e6
---

# ADR-002 Repo layout

## Context

Нужна ясная структура monorepo vs split engine/game.

## Decision

Один репозиторий `rat-engine` на старте; game sample/app живёт рядом (`apps/` или `examples/`). Split — только если появится отдельный продукт-релиз движка.

## Consequences

- Проще синхронизировать API и vertical slice.
- Позже можно выделить пакеты/crates без смены git history strategy вслепую.
