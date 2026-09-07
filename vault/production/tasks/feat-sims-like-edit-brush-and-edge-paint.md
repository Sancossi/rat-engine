---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Sims-like edit brush and edge paint

Epic / parent. Слайсы (порядок):

1. [[research-sims-build-mode|research: Sims build-mode edit analog]]
2. [[feat-edit-hold-drag-brush|feat: Edit hold-drag brush]] — кисть по клеткам
3. [[feat-edit-paint-clicked-edge|feat: Edit paint clicked edge]] — рамки с любой стороны + drag вдоль стены; закрывает [[edit-edges-only-one-facing]]

Intent: переделать Edit mode ближе к The Sims build. Зажать кнопку и вести — кисть по нескольким клеткам (куб / высота / плита). Стены (рамки) — по **кликнутому ребру** клетки, drag вдоль стены, все четыре стороны без combo-по-умолчанию.

Acceptance: см. слайсы. Does not replace Play.

Origin: chat 2026-09-02. Related: [[feat-mouse-viewport-terrain-edit|feat: Mouse viewport terrain edit]], [[collect-5-reference-games|Collect 5 reference games]]. Взято в [[Sprint 8 — Play feel and authoring]].

## Resolution

Слайсы shipped: Sims research, hold-drag cube/slab, fence/ladder по кликнутой грани. Verify: Edit кисть + Fence у края клетки.

## Bugs found

none.
