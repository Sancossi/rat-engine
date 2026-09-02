---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Event graph model and compile

Intent: модель графа (ноды + рёбра) и compile → существующий `EventPage` / `Command`. Play не читает граф. Headless round-trip JSON.

Acceptance: serialize/load graph; compile matches today’s linear list for a golden page; invalid graph = structured error, карта не стартует. Нет ImGui canvas.

Origin: [[feat: Event node graph authoring]]. Depends: [[research: Event node graph vs bytecode]]. Взято в [[Sprint 8 — Play feel and authoring]].
