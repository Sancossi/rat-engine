---
type: task
area: Engine
status: Not started
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: PlaySE review test polish

Intent: minor из ревью [[feat: PlaySE event command]] (Approved): round-trip не опирается на `find("\"id\"")`; есть кейс пустого `play_se` `id` (loader reject); dump пустого cue либо зеркалит parse, либо не возникает из JSON-круга.

Acceptance: `[playse]` ломается на пустом id; serialize/parse assert не ложно-зелёный из-за чужого `"id"`.

Origin: [[feat: PlaySE event command]]
