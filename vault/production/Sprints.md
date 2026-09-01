---
type: index
tags: [sprint]
---

# Sprints

Один спринт = один файл в `production/sprints/`. Ровно у одного файла `current: true`.

```dataview
TABLE status, dates, goal, current
FROM "production/sprints"
WHERE type = "sprint"
SORT file.name ASC
```
