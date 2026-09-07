---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: notify bus review polish

Intent: minor из ревью [[feat-gameplay-notify-observer|feat: Gameplay notify observer]] (Approved): тест на FIFO двух подписчиков; `post` не итерирует `handlers_` на месте (снимок перед вызовом); `Landed` при same-substep relaunch после касания — либо постится, либо задокументирован как не-кейс.

Acceptance: `[notify]` ломается, если второй handler вызывается первым; подписка из handler не UB.

Origin: [[feat-gameplay-notify-observer|feat: Gameplay notify observer]]

## Resolution

`GameplayNotifyBus::post()` copies `handlers_` then iterates the snapshot, FIFO. Subscribe during `post` is not invoked for that notify (same contract as audio drain). `[notify]` covers two subscribers / one post (order vector) and subscribe-during-post. `PlayerFrameResult::landed` stays sticky if a later substep takes off; jump physics unchanged. Review: Approved.

## Bugs found

none.
