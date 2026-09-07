---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Edit submodes terrain objects events

Intent: в Edit один плоский набор tools и все панели сразу. Нужны три подрежима: Terrain / Objects / Events — каждый со своим pick и панелями.

Acceptance: Inspector переключатель Terrain | Objects | Events. Смена подрежима сбрасывает tool в Select и несовместимый selection. Terrain: cube/fence/slab + height/ramps, pick не выбирает events/blockers. Objects: blocker/ladder + blocker panel и ladder UI (вынести из terrain). Events: только events. Ladder pick в 3D не делать. Не путать с Play/Edit (`AppMode`).

Origin: chat 2026-09-03 (план Edit submodes и окно графа). Follow-up: [[feat-event-names-in-edit-viewport|feat: Event names in edit viewport]], [[feat-event-rmb-create-edit-copy-delete|feat: Event RMB create edit copy delete]], [[feat-event-graph-window-pages-and-conditions|feat: Event graph window pages and conditions]].

## Resolution

`EditSubmode` {Terrain, Objects, Events} фильтрует pick и tool row. Terrain кликает сквозь blockers/events (Place cube/fence/slab). Objects — только blockers + ladder panel. Events — только events. Смена подрежима сбрасывает Select и чужой selection. Verify: F2 → Inspector Terrain|Objects|Events; `.\build\tests\rat_tests.exe "[viewport_edit]"`. Review: Approved.

## Bugs found

none.
