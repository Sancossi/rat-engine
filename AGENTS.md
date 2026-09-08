# Working on rat-engine

The product and work queue live in `vault/`; engineering contracts live in `docs/`.
Read the current card and its referenced specification before implementation.
Use `rg` to deduplicate durable requests before creating a task or bug from the
templates. Pure questions and implementation details within a card need no new card.

An audit/review request authorizes inspection, reproduction, and scoped reports or
finding cards, not implementing runtime fixes or automatically starting the sprint
queue. Only an implementation request activates execution below.

## Execution and review

- Use a feature branch and commit coherent, verified slices. Preserve unrelated
  working-tree changes; never stage the whole tree indiscriminately.
- For a sprint, plan, or multi-step card, delegate to one implementer at a time
  in the shared tree, followed by an independent read-only reviewer. The parent
  owns card status changes. Typo and status-only edits may stay with the parent.
- Before dispatch, set tasks to `In progress`, bugs to `Investigating`, and
  commit. Implementers commit their changes but do not close or advance cards.
- For task review use `status: In review`. Bugs remain `Investigating` with
  `review: In review`. After approval use task `Done` or bug `Fixed` and record
  `review: Approved`. Needs fixes returns tasks to `In progress`; bugs remain
  `Investigating` with `review: Needs fixes`.
- Continue the approved sprint order without asking for each next card. Only
  one implementer may write; read-only review may overlap independent work.
  Stop pickup on a blocking dependency, negative review, scope change, or user stop.
- At closure write Resolution (behavior, validation evidence) and Bugs found
  (`none` if appropriate). Link newly discovered work bidirectionally with
  Origin / Follow-up; schedule only in-scope followups into the current sprint.
- Rebuild the full Release editor after closing each stage and report its path.
  Never claim GUI or remote CI verification from headless tests alone.

## Vault contract

There may be zero current sprints between execution periods and exactly one during
execution. A current sprint has `current: true` and `status: In Progress`.
The Current sprint view in `vault/production/Task board.base` must match its short
label (`Sprint N`); when none is active use `__NO_ACTIVE_SPRINT__`. Include tasks
and bugs in the board. Closed predecessors stay closed.

Use actual filenames for wikilinks: `[[feat-mouse-viewport-map-edit|Mouse viewport map edit]]`.
A note's H1 is a display label, not a link target. Qualify ambiguous names with a
vault-relative path. Keep historical notes and provenance; do not create empty
notes merely to silence broken links.

Required scalar fields and enum values are enforced by `scripts/check_vault.py`:
tasks use `Not started / In progress / In review / Blocked / Done / Archived`;
bugs use `Open / Investigating / Fixed / Wont Fix`. Bug review is a separate field,
not a task status. Copy new-note templates from `vault/templates/`.

## Verification and architecture

- Windows: `powershell -ExecutionPolicy Bypass -File scripts/verify.ps1` discovers
  the C++ toolchain and performs configure, build, tests, and vault checks.
- Portable: `cmake --preset dev-release`, `cmake --build --preset dev-release`,
  `ctest --preset dev-release`; Python 3: `python scripts/check_vault.py` and
  `python -m unittest discover -s scripts/tests`.
- Keep graphics/platform dependencies outside `rat_core` public interfaces;
  preserve the CMake link-isolation check. Runtime and authoring currently share
  `rat_core`; proposed target libraries in the roadmap are not implemented targets.
- Prefer regression scenarios that reproduce observable failures. Reversible
  documentation changes need the vault check, not new C++ tests.
- Project skills in `.agents/skills/` cover build verification, vault maintenance,
  and runtime reproduction. They supplement this contract without broadening scope.

## BMad game workflow

- BMad Core and Game Dev Studio supplement this contract. Setup and real skill
  names are in `docs/bmad/README.md`; load `docs/bmad/project-context.md` before
  using them. Use the approved design inputs instead of repeating settled intake.
- `vault/game/GDD.md` is the canonical design; companion documents live in
  `vault/game/expedition/`. Engineering specifications and review evidence live
  in `docs/`. Every new vault Markdown note needs valid scalar frontmatter.
- Vault cards remain the only editable task/status source. Before native sprint
  reporting run `python scripts/bmad_vault.py sync` then `check`. Never edit the
  generated projection or create a parallel story queue; it does not authorize
  pickup, mark blocked work ready, or replace independent review.
