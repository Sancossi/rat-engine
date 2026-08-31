# Obsidian Task Board Design

## Goal

Give humans a kanban-style board of vault tasks in Obsidian without a second
card database. YAML frontmatter on files in `vault/production/tasks/` stays the
source of truth for agents.

## Design

One Obsidian Base, `vault/production/Task board.base`, filters to
`production/tasks/` notes with `type: task`. Three views in that file:

- **Board** — cards grouped by a `board_column` formula so status order is
  Not started → In progress → Done. Archived is hidden.
- **Sprints** — cards grouped by a `sprint_column` formula. Empty `sprint` is
  **Backlog**. Archived is hidden.
- **Table** — flat table of all tasks including Archived (replaces the Dataview
  table on `[[Tasks]]`).

Cards show area, task_type, and the property that is not the group
(sprint on Board, status on Sprints). Changing a property in the Base UI writes
frontmatter. No drag-and-drop plugin in this version.

`[[Tasks]]` embeds the base and keeps the field legend plus the agent Grep
hint. Wikilink is `[[Task board]]` so it does not collide with `Tasks.md`.

Out of scope: bugs board, community Kanban plugin.

Humans looking at **what is in flight** use the **In progress** view. **Current sprint** is the Sprint N kanban (filter `sprint == "Sprint N"`; update when `current: true` moves).

## Acceptance

- Opening `Task board.base` (or the embed on `[[Tasks]]`) shows Board / Sprints /
  Table.
- Board columns follow workflow order, not alphabetical status.
- Sprints view groups by `sprint`; unscheduled tasks sit under Backlog.
- Editing status or sprint in the Base updates the note YAML.
- Agent queries still Grep `vault/production/tasks/` frontmatter.
