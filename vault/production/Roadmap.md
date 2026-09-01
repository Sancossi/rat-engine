---
type: index
tags: [roadmap]
---

# Roadmap

Milestones в `production/roadmap/`. Поля: `priority` (P0–P3), `status` (Idea / Planned / In Progress / Done), `target`, `area`.

```dataview
TABLE area, priority, status, target
FROM "production/roadmap"
WHERE type = "roadmap"
SORT priority ASC, file.name ASC
```
