---
type: index
tags: [adr]
---

# Decisions

ADR в `production/decisions/`. Статусы: `Proposed` | `Accepted` | `Superseded`.

```dataview
TABLE area, status, decided
FROM "production/decisions"
WHERE type = "adr"
SORT file.name ASC
```
