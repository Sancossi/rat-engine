---
type: task
area: Engine
status: In progress
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: notify bus review polish

Intent: minor из ревью [[feat: Gameplay notify observer]] (Approved): тест на FIFO двух подписчиков; `post` не итерирует `handlers_` на месте (снимок перед вызовом); `Landed` при same-substep relaunch после касания — либо постится, либо задокументирован как не-кейс.

Acceptance: `[notify]` ломается, если второй handler вызывается первым; подписка из handler не UB.

Origin: [[feat: Gameplay notify observer]]
