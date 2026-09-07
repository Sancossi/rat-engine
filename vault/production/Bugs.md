---
type: index
tags: [bug]
---

# Bugs

Один баг = один файл в `production/bugs/`. Поля: `severity` (Critical / High / Medium / Low), `status` (Open / Investigating / Fixed / Wont Fix).

Агент: Grep `type: bug` и `status: Open`.

Проверка отдельным полем `review`: Pending / In review / Approved / Needs fixes.
При ревью баг остаётся Investigating; после Approved становится Fixed.

```dataview
TABLE area, severity, status
FROM "production/bugs"
WHERE type = "bug"
SORT severity ASC
```
