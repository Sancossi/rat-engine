---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: RenderWorld packets and instrumentation

Intent: `SimulationSession` → `RenderWorld` → `RenderPackets` → passes → bgfx. Render-only snapshot с интерполяцией; сортировка packets и material handles; frame allocator; отложенный free GPU; CPU/GPU markers, debug names, RenderDoc hook. Свой RHI не делать. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: renderer не принимает `GameState` / `EventRuntime` / editor-типы; snapshot строится без вызова bgfx; основные passes видны в profiler/capture.

Depends: [[research-entity-model-ecs-vs-scene|research: Entity model ECS vs scene vs hybrid]]. Next: [[chore-native-window-platform-adapters|chore: NativeWindow platform adapters]].

Origin: merged `docs/archive/cpp/architecture-roadmap.md` (2026-09-01).

## Resolution

`SimulationSession` собирает CPU-снимок `RenderWorld` (пакеты, сортировка, имена pass Depth/Opaque/Debug) без bgfx. `Renderer::submit` принимает только `const RenderWorld&`. Editor `present()` гоняет вертикаль в существующий bgfx; greybox пока рисует геометрию. Verify: `.\build\tests\rat_tests.exe "[render]"` и `ctest`. Review: Approved.

## Bugs found

none.
