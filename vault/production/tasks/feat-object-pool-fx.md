---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Object pool for short-lived FX

Intent: GPP Object Pool, когда появятся пули/хиты/партиклы ([[ADR-005 Conflict model exploration plus field action]]). До field-action FX не делать.

Acceptance: пул с явным acquire/release; overflow warn, без скрытого new в горячем пути; тест на reuse.
