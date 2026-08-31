---
type: adr
area: Production
status: Accepted
decided: 2026-08-31
tags: [adr]
---

# ADR-011 Knowledge hub lives in git (Obsidian vault)

## Context

Хаб (Engine / Game / Wiki / Tasks / Bugs / Sprints / Roadmap / Decisions) жил в Notion. Агент писал туда через MCP; репозиторий не содержал источник правды по продукту.

## Decision

Источник правды — папка `vault/` в репозитории `rat-engine`, открываемая как Obsidian vault. Задачи и баги — markdown + YAML. Агент читает и пишет файлы; Notion MCP для этого репо не используется.

## Consequences

- Идеи из чата попадают в git, а не теряются в Notion.
- Diff задач/ADR ревьюится как код.
- Человек открывает `vault/` в Obsidian; доска задач — Bases (`Task board.base`); Dataview опционален на остальных индексах.
- Существующий `docs/` остаётся для схем и implementation specs.
