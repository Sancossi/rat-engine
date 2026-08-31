---
name: obsidian-vault
description: >-
  Reads and writes the rat-engine Obsidian vault at vault/. Use when the user
  mentions notes, wiki, GDD, ADR, vault, Obsidian, tasks, bugs, sprints,
  roadmap, or knowledge hub; when capturing ideas; or when looking up product
  or production decisions. Do not use Notion MCP for this repository.
---

# Obsidian vault (rat-engine)

Source of truth for the product hub is `vault/` in this repo. Open it in Obsidian as a vault (File → Open folder as vault → `vault/`).

**Do not write to Notion.** Do not create/update Notion pages, databases, or comments for rat-engine work.

Engineering artifacts stay in `docs/` (schemas, acceptance, `docs/superpowers/specs/`). Hub knowledge lives in `vault/`.

## Map

| Path | Role |
| --- | --- |
| `vault/HOME.md` | Hub MOC |
| `vault/engine/` | Vision, Architecture, Systems Index, Conventions |
| `vault/game/` | GDD, Narrative, Art, Audio, Event System |
| `vault/wiki/` | How we work, Glossary, Meeting Notes |
| `vault/production/tasks/` | One task per file |
| `vault/production/Task board.base` | Human kanban: Current sprint / In progress / Board / Sprints / Table |
| `vault/production/bugs/` | One bug per file |
| `vault/production/sprints/` | One sprint per file; exactly one has `current: true` |
| `vault/production/roadmap/` | Milestones |
| `vault/production/decisions/` | ADRs |
| `vault/templates/` | New-note skeletons |

## Wikilinks

Use Obsidian wikilinks: `[[GDD]]`, `[[ADR-001 Language and runtime]]`, `[[Sprint 3 — Height-grid traversal]]`. Prefer the note heading / filename, not Notion URLs.

## Frontmatter

Copy fields from [templates.md](templates.md). Required `type`:

- `task` — `area`, `status`, `task_type`, optional `sprint` / `roadmap` / `due`
- `bug` — `area`, `status`, `severity`, optional `sprint`
- `sprint` — `status`, `dates`, `goal`, `current` (boolean)
- `roadmap` — `area`, `priority`, `status`, `target`
- `adr` — `area`, `status`, `decided`

Enums match the old Notion boards:

- area: `Engine` | `Game` | `Production`
- task status: `Not started` | `In progress` | `In review` | `Blocked` | `Done` | `Archived`
- task_type: `Feature` | `Bug` | `Chore` | `Research`
- bug status: `Open` | `Investigating` | `Fixed` | `Wont Fix`
- bug severity: `Critical` | `High` | `Medium` | `Low`

Filenames: lowercase slug from title (`s2-play-edit-mode-toggle.md`). Title is the `#` heading.

`notion_id` is migration provenance only. Do not add it on new notes.

## Create / update

1. Search the vault (Grep/Glob) before creating — update a close match instead of cloning.
2. New task/bug: copy `vault/templates/task.md` or `bug.md`.
3. After create, reply with the **repo-relative path** and one-line title.

For listing current sprint, backlog, or open bugs, follow **obsidian-query**.
