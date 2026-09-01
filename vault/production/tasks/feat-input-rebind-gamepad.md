---
type: task
area: Engine
status: In review
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: Input rebind and gamepad

Intent: ребинд клавиш и геймпад поверх [[feat: Input action mapping]]. В первой карточке ввода этого нет.

Acceptance: те же `InputFrame` actions с другой раскладки и с геймпада; дефолт WASD сохраняется; bindings сериализуются; без окна — тест маппинга. Keyboard/gamepad adapters; в симуляцию уходит только `InputFrame`.

Depends: [[chore: NativeWindow platform adapters]]. Next: [[research: Audio backend ADR]]. Взято в [[Sprint 7 — Engine architecture]].

Origin: [[feat: Input action mapping]] + merged `docs/architecture-roadmap.md` (2026-09-01).
