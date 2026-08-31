---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 4
due:
tags: [task]
---

# feat: Audio play-queue stub

Intent: GPP Event Queue для звука + явный интерфейс `Audio` (не locator, не синглтон), чтобы геймплей постил PlaySfx без знания бэкенда. Контекст: сверка [[Game Programming Patterns]] (2026-08-31); продукт — [[Audio Direction]]. Выбор miniaudio/OpenAL — отдельный ADR позже. Opcode `PlaySE` в событиях — follow-up.

Acceptance: очередь `PlaySfx` / `PlayMusic` / `Stop`; null/log sink; drain раз в кадр из shell; геймплей не знает бэкенд.
