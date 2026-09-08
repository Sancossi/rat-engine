---
type: task
area: Production
status: Done
task_type: Chore
sprint: Sprint 16
due:
review: Approved
tags: [task, bmad, preproduction]
---

# Install BMad and integrate the authoritative vault workflow

Intent: Implement the approved process for the new rat expedition RPG.

Specification: [Approved plan](../../../docs/rat-expedition-preproduction-plan.md).

Acceptance:
- Stable BMad Core and full Game Dev Studio installed locally, versions recorded and reinstall documented.
- Russian output; canonical vault documents and statuses integrated without a second manual queue.
- Workflow discovery, a documentation smoke scenario and adapter checks pass.
- User changes preserved; independent review approved.

Origin: User-approved game/process redesign, 2026-09-08; [[Sprint 16 — Rat expedition preproduction]].

Follow-up: [[design-rat-expedition-preproduction]].

## Resolution

Implemented in commit `6ce8ec5`. BMad Core 6.12.0 and full GDS v0.7.2 provide
41 project-local skills, pinned setup/reinstall, Russian configuration and a
one-way projection of vault statuses. [Integration evidence](../../../docs/bmad/integration-smoke.md)
records fresh and existing-project setup, real workflow resolvers and limitations.

Independent read-only reviewer approved `6ce8ec5` on 2026-09-08 with no actionable
findings: smoke, projection check, 16 Python tests and vault validation pass;
40/40 pre-existing user files remain byte-identical and unstaged. Parent completed
full Release verification at Sprint 16 closure: 714 C++ and 16 Python tests pass.
[Stage evidence](../../../docs/audits/2026-09-08-rat-expedition-preproduction.md). Interactive client
reload, gameplay, GUI and remote CI were not verified by this documentation slice.

## Bugs found

none remaining. Installation action handling and resolver shape discovered during
local smoke were corrected before review; evidence records the tested adaptations.
