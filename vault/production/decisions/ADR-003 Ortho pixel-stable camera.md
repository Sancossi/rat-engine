---
type: adr
area: Engine
status: Accepted
decided: 2026-08-31
tags: [adr]
notion_id: 3ccf3827-36cc-81db-978f-caa9391554c9
---

# ADR-003 Ortho pixel-stable camera

## Context

Нужна камера и презент под stylized 3D pixel без ряби.

## Decision

MVP: **orthographic 3/4**, render to integer-scaled low-res (или texel-snapped) buffer, point sampling для pixel albedo, camera snap to pixel/texel grid.

## Consequences

- Perspective follow-cam откладывается.
- Арт пайплайн обязан соблюдать texel density.
- Шейдеры/post ориентированы на стабильный upscale, не на TAA.
