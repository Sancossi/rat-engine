# Rat expedition: approved preproduction plan

Approved by the user on 2026-09-08. This is the execution contract for
`chore-bmad-vault-integration` and `design-rat-expedition-preproduction`.

## Intent and accepted constraints

Replace the previous game concept and development process with BMad Core plus
the complete Game Dev Studio module. Deliver documentation and a usable queue;
do not start implementing the game or automatically resume legacy sprints.

The new game is a single-player, 1–2 hour Windows RPG, developed by one person
with AI, a minimal budget and a few hours per day. Up to three sentient rat heroes
explore abandoned medieval holdings to find a magical relic. Tone: dark fantasy.
Small interconnected locations, traversal and systematic item/environment
interactions, consequential branching dialogue, menu-based party turn combat
without a tactical grid, 2D characters in a 3D world.

References: old Final Fantasy and Breath of Fire (world and presentation), MGS
(traversal and systemic interaction), Disco Elysium (dialogue choices and their
consequences), Dead Cells (visual style). Do not import unrelated mechanics.

The user selected engine choice after concept definition. Compare rat-engine,
Godot and Unity against these requirements, provide evidence and a recommendation,
but keep engine adoption as a subsequent decision, not an implicit migration.

## Slice 1: BMad integration

Install stable BMad Core and the complete Game Dev Studio module using the
official installer with project-local integration. Pin/record package and module
versions and a reproducible installation method. Russian conversation and
document output. No global installation, automatic updates or extra frameworks.

Vault is the authoritative product and task/status store; docs holds technical
specifications and verification evidence. Configure BMad accordingly. If native
BMad workflow summaries require a second status file, derive it one-way from vault
with explicit status mapping; never keep two manually editable queues. Respect
existing scalar frontmatter, task/bug status and review rules, project ownership
and the single implementer/read-only reviewer workflow. Add focused verification
for any adapter logic, including task/bug mappings and stale-output detection.

Verify actual installed workflow discovery and one honest documentation smoke
scenario: design -> task -> review evidence. Do not claim interactive tool reload
or gameplay testing merely from static checks. Preserve upstream licenses.

## Slice 2: product and production documents

Create a substantive Russian concept brief, canonical updated GDD, narrative,
art/audio direction including affordable asset workflow, sourced technical engine
comparison, milestone roadmap, first-prototype detailed tasks and later epic tasks.
Explicitly label author-proposed defaults and prototype hypotheses rather than
claiming that the user decided invented details. GDD must describe usable rules
and the first scenario, not only headings and promises to design them later.

Roadmap: preparation -> engine decision -> prototype -> 15–20 minute polished
vertical slice -> whole expedition production -> release preparation. Gate each
stage on demonstrable behavior. Include alternative quest solution, dialogue
consequences, combat ending, save/load consistency, and human playtesting.
Estimate calendar commitments only after measured prototype throughput.

Document replacement of previous no-magic/no-turn-combat/3D-character assumptions
without rewriting history. Assess unfinished old work for reuse, defer or replace;
do not mass-close or activate legacy cards. New future work remains Not started
outside the active preparation sprint. Link Origin and Follow-up bidirectionally.

## Preservation and acceptance

User had many dirty vault and editor configuration files before execution.
Do not edit or stage those files. Use new companion notes plus clean entrypoints;
preserve baseline bytes. Parent owns all task/sprint status changes. Implementers
commit only their explicitly scoped files. Closed historical sprints stay closed.

Run vault validation and Python checks, meaningful integration smoke checks, and
independent read-only review. At stage closure rebuild the full Release editor
using scripts/verify.ps1 and record exact executable path, command outcomes and
limitations. No runtime changes, GUI claims or game asset generation are needed.
