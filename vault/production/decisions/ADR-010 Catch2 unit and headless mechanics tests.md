---
type: adr
area: Engine
status: Accepted
decided: 2026-08-31
tags: [adr]
notion_id: 3cdf3827-36cc-81e5-aa8c-f8d80f1a5123
---

# ADR-010 Catch2 unit and headless mechanics tests

## Context

Нужны unit-тесты и тесты механик с первого спринта без зависимости от GPU/окна.

## Decision

- **Catch2** via FetchContent; target `rat_tests` + `ctest`
- Разделить **`rat_core`** (logic, no bgfx/GLFW) и **`rat_engine`** (render)
- Unit: GameState и будущий Event runtime
- Mechanics: headless scenario runner (без окна)
- Render/UI smoke — отдельно и не блокер CI MVP

## Consequences

- Новая логика игры кладётся в `rat_core`.
- `RAT_BUILD_TESTS` ON по умолчанию.
