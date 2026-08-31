---
type: task
area: Engine
status: Not started
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: undo grow/shrink and field edits

Intent: place/move/delete уже в [[feat: Edit undo/redo command stack]]; grow/shrink AABB, jumpable/vertical range и правки текста/страниц события всё ещё необратимы (YAGNI той карточки).

Acceptance: те же `EditHistory` команды (или replace-at-index) для resize/jumpable/page-text; Play по-прежнему не пишет стек.

Origin: [[feat: Edit undo/redo command stack]]
