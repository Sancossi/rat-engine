---
type: task
area: Engine
status: Done
task_type: Chore
sprint: Sprint 4
due:
tags: [task]
---

# chore: PlaySE review test polish

Intent: minor из ревью [[feat-play-se-event-command|feat: PlaySE event command]] (Approved): round-trip не опирается на `find("\"id\"")`; есть кейс пустого `play_se` `id` (loader reject); dump пустого cue либо зеркалит parse, либо не возникает из JSON-круга.

Acceptance: `[playse]` ломается на пустом id; serialize/parse assert не ложно-зелёный из-за чужого `"id"`.

Origin: [[feat-play-se-event-command|feat: PlaySE event command]]

## Resolution

`[playse]` round-trip asserts `"play_se"` and `"id": "jump"` (not a bare `"id"`, which also matches the event id `"sfx"`). Loader rejects `{ "op": "play_se", "id": "" }` (`ok == false`, error non-empty). Parse already threw `play_se id must not be empty`; dump-side empty-id check not added (JSON round-trip cannot produce empty id). Runtime PlaySE unchanged. Review: Approved.

## Bugs found

none.
