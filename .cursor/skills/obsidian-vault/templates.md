# Vault frontmatter templates

New files: copy from `vault/templates/` then fill. Do not invent extra required keys.

## Task (`vault/templates/task.md`)

```yaml
---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---
```

`sprint` values are short labels: `Sprint 0`, `Sprint 1`, `Sprint 2`, `Sprint 3` (match the sprint note title prefix). Leave empty unless the user ties the work to the current sprint.

## Bug (`vault/templates/bug.md`)

```yaml
---
type: bug
area: Engine
status: Open
severity: Medium
sprint:
tags: [bug]
---
```

Body: Repro / Expected / Actual. Set `sprint` for follow-up bugs discovered
during current-sprint work; otherwise leave it empty.

## ADR (`vault/templates/adr.md`)

```yaml
---
type: adr
area: Engine
status: Proposed
decided:
tags: [adr]
---
```

Filename: `ADR-NNN Title.md` next to existing decisions. Next number after the highest `ADR-0xx` in `vault/production/decisions/`.

## Note (`vault/templates/note.md`)

```yaml
---
type: note
tags: []
---
```
