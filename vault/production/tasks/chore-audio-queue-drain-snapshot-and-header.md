---
type: task
area: Engine
status: In progress
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

`QueuedAudio::drain()` moves `queue_` into a local batch before `apply`, so posts from a sink (including nested `play_*` / `stop` / `drain`) wait for the next `drain()` — no in-place iteration, no drain-until-empty loop. `audio.hpp` forward-declares `Logger`; `log.hpp` is included from `audio.cpp` and from the LogAudioSink test. Card stays In progress pending review.

## Bugs found

none.
