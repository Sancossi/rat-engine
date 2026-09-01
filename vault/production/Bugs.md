---
type: index
tags: [bug]
---

# Bugs

Один баг = один файл в `production/bugs/`. Поля: `severity` (Critical / High / Medium / Low), `status` (Open / Investigating / Fixed / Wont Fix).

Агент: Grep `type: bug` и `status: Open`.

```dataview
TABLE area, severity, status
FROM "production/bugs"
WHERE type = "bug"
SORT severity ASC
```
