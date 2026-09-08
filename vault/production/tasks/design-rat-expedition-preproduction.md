---
type: task
area: Game
status: Done
task_type: Feature
sprint: Sprint 16
due:
review: Approved
tags: [task, gdd, preproduction]
---

# Write the new rat RPG design and release roadmap

Intent: Turn the accepted concept into an actionable design and bounded release queue.

Specification: [Approved plan](../../../docs/rat-expedition-preproduction-plan.md).

Acceptance:
- Russian concept, GDD, narrative, art/audio direction and sourced engine comparison are usable and mutually consistent.
- Preparation-to-release roadmap has acceptance gates; prototype tasks have dependencies and observable checks.
- Historical decisions and user modifications preserved; proposed defaults distinguished from accepted requirements.
- New queue remains separate from legacy pickup; vault validation and independent review pass.

Origin: [[chore-bmad-vault-integration]]; [[Sprint 16 — Rat expedition preproduction]].

Follow-up:

- [[expedition-engine-decision]]
- [[expedition-prototype-traversal]]
- [[expedition-prototype-interactions]]
- [[expedition-prototype-quest-dialogue]]
- [[expedition-prototype-combat]]
- [[expedition-prototype-save-load]]
- [[expedition-prototype-integrated-scenario]]
- [[expedition-prototype-playtest]]
- [[expedition-vertical-slice]]
- [[expedition-full-production]]
- [[expedition-release-readiness]]

## Resolution

Implemented in `3edeadd` and `9deb9c1`: [[rat-expedition|new game hub]], [[GDD]],
brief, narrative, art/audio, canonical ADR-016 and release roadmap, sourced engine
comparison, exact historical GDD archive and assessment of all six unfinished
legacy cards. Eleven linked future tasks remain Not started outside any sprint.
Godot is a provisional recommendation; engine adoption remains a future decision.

Independent read-only reviewer approved both commits on 2026-09-08 with no
actionable findings. GDS quality, discipline, RPG genre, structural and scope
checklists pass, including accepted-input fidelity and A1–A8 proposal tracking.
Vault/projection checks and 16 Python tests pass. All 40 initial and 7 later
external user files remain byte-identical; historical GDD content is preserved.

Approval covers documentation and preparation, not gameplay or balance. The
parent performs full Release editor verification at Sprint 16 closure.

## Bugs found

none in independent review. A newly appeared empty wiki note collided with the
roadmap filename during editing; it was preserved as a meaningful navigation note
and new links were qualified with the canonical vault path. No legacy runtime bug
was fixed and no new game was executed in this slice.
