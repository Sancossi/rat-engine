---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Grid edge barriers

Intent: забор на ребре клетки `{tile, direction, height}` над верхом владельца. Мини vs полная — одна высота vs апекс прыжка (~1.17). Ребро не support. Origin: [[feat: Terrain wall cubes]].

Acceptance: schema v2 `edge_barriers` (без bump v3); лоадер канонизирует дубликаты и режет ramp-клетки; ходьба через ребро блокируется; 0.45 перепрыгивается; 1.6 нет даже с полным hold.

Depends: [[feat: Terrain wall cubes]]. Next: [[feat: Edit and greybox edge walls]].
