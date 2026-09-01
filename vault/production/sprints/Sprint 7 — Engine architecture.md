---
type: sprint
status: Active
dates: 2026-10-13/2026-10-26
goal: Split simulation, authoring, render, and platform adapters per architecture roadmap
current: true
tags: [sprint]
---

# Sprint 7 — Engine architecture

Цель: нарезать вмерженный [architecture roadmap](../../../docs/architecture-roadmap.md) в одну очередь PR. Симуляция, authoring, render и платформа — отдельные слои; без ECS/RHI/job system до замера.

DoD:

- CI собирает `rat_core`, editor и Catch2 на Windows и Linux; headless smoke `grey_yard`; `rat_core` не линкует GLFW/ImGui/bgfx
- editor и headless делят один `SimulationSession::tick(InputFrame)`; replay даёт тот же checksum
- `EventRuntime` не мутирует карту; JSON → validation → `MapDocument` → `RuntimeMap`
- `EditorApp` — composition root; панели не зовут mutation `EventRuntime`
- карты ссылаются на `AssetId`; принят entity-model ADR; renderer ест `RenderPackets`, не `GameState`
- platform/input/audio за адаптерами; есть метрики tick / draw calls / queue depth

Вне скоупа: archetype ECS, универсальный RHI, job system, networking. [[feat: Spatial partition broadphase]] и [[feat: Object pool for short-lived FX]] — бэклог до подтверждённого bottleneck (этап 9 roadmap).

Порядок:

- [[chore: CI Windows/Linux and grey_yard smoke]] → [[chore: rat_core isolation from platform graphics]] → [[feat: SimulationSession unified tick]] → [[feat: Replay recording and checksum]] → [[feat: MapDocument and RuntimeMap compile]] → [[jump-button-does-not-always-fire]] → [[ramp-climb-does-not-work]] → [[chore: Decompose EditorApp]] → [[feat: Asset registry and first load vertical]] → [[research: Entity model ECS vs scene vs hybrid]] → [[feat: RenderWorld packets and instrumentation]] → [[chore: NativeWindow platform adapters]] → [[feat: Input rebind and gamepad]] → [[research: Audio backend ADR]] → [[feat: Audio backend implementation]] → [[chore: Engine frame metrics]]
