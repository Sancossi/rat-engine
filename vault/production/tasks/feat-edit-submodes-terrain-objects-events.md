---
type: task
area: Engine
status: In progress
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Edit submodes terrain objects events

Intent: в Edit один плоский набор tools и все панели сразу. Нужны три подрежима: Terrain / Objects / Events — каждый со своим pick и панелями.

Acceptance: Inspector переключатель Terrain | Objects | Events. Смена подрежима сбрасывает tool в Select и несовместимый selection. Terrain: cube/fence/slab + height/ramps, pick не выбирает events/blockers. Objects: blocker/ladder + blocker panel и ladder UI (вынести из terrain). Events: только events. Ladder pick в 3D не делать. Не путать с Play/Edit (`AppMode`).

Origin: chat 2026-09-03 (план Edit submodes и окно графа). Follow-up: [[feat: Event names in edit viewport]], [[feat: Event RMB create edit copy delete]], [[feat: Event graph window pages and conditions]].
