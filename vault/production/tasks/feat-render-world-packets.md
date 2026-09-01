---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: RenderWorld packets and instrumentation

Intent: `SimulationSession` → `RenderWorld` → `RenderPackets` → passes → bgfx. Render-only snapshot с интерполяцией; сортировка packets и material handles; frame allocator; отложенный free GPU; CPU/GPU markers, debug names, RenderDoc hook. Свой RHI не делать. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: renderer не принимает `GameState` / `EventRuntime` / editor-типы; snapshot строится без вызова bgfx; основные passes видны в profiler/capture.

Depends: [[research: Entity model ECS vs scene vs hybrid]]. Next: [[chore: NativeWindow platform adapters]].

Origin: merged `docs/architecture-roadmap.md` (2026-09-01).
