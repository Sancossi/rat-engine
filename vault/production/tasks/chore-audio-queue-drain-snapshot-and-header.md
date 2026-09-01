---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: audio queue drain snapshot and header include

Intent: minor из ревью [[feat: Audio play-queue stub]] (Approved): `drain()` не итерирует `queue_` на месте (снимок/`move` перед `apply`); `audio.hpp` не тянет полный `log.hpp`, если достаточно forward-declare `Logger`.

Acceptance: reentrant `play_*` из `apply` безопасен или задокументирован; потребители `Audio` без log-типов компилируются.

Origin: [[feat: Audio play-queue stub]]

## Resolution

`QueuedAudio::drain()` moves `queue_` into a local batch before `apply`. Posts from `apply` land on the empty `queue_`. If `apply` itself calls `drain()`, that nested call is the next drain and applies those posts. No in-place iteration, no drain-until-empty loop. `audio.hpp` forward-declares `Logger`; `log.hpp` lives in `audio.cpp` and the LogAudioSink test. Review: Approved.

## Bugs found

none.
