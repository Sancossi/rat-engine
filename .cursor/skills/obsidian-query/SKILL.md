---
name: obsidian-query
description: >-
  Lists and filters rat-engine vault boards via Grep/Glob on YAML frontmatter.
  Use when the user asks for current sprint, backlog, in-progress work, open
  bugs, roadmap, or ADRs. Do not query Notion.
---

# Query the vault

All queries are filesystem search under `vault/`. Do not use Notion MCP.

## Current sprint

1. Glob `vault/production/sprints/*.md`
2. Grep `current: true` — one match during execution, zero allowed between stages; more than one is an error (see `AGENTS.md`).
3. Read that note for Goal / DoD
4. Grep `sprint: Sprint N` in `vault/production/tasks/` using the short label from the sprint heading (`Sprint 3` for `Sprint 3 — Height-grid traversal`)

## Backlog

Grep `status: Not started` in `vault/production/tasks/`. Open work: also `In progress`, `In review`, `Blocked`.

Exclude `status: Done` and `status: Archived`.

## Open bugs

Grep `type: bug` in `vault/production/bugs/`, then keep `status: Open` or `status: Investigating`.

## Roadmap / ADR

- Roadmap: `vault/production/roadmap/`, filter `status:` / `priority:`
- Decisions: `vault/production/decisions/`, filter `status: Accepted` etc.

## Output

Reply with note titles + repo-relative paths. Quote frontmatter fields, not Notion URLs.

Human boards in Obsidian: `Task board.base` views **Current sprint** and **In progress**. The agent must not depend on Bases or Dataview; Grep frontmatter.
