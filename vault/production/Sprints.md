---
type: index
tags: [sprint]
---

# Sprints

Один спринт = один файл в `production/sprints/`. Во время исполнения ровно один
имеет `current: true`; между этапами допустимо ни одного. Правила: [AGENTS.md](../../AGENTS.md).

```dataview
TABLE status, dates, goal, current
FROM "production/sprints"
WHERE type = "sprint"
SORT file.name ASC
```
