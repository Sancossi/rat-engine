---
type: sprint
status: Done
dates: 2026-10-13/2026-10-26
goal: Split simulation, authoring, render, and platform adapters per architecture roadmap
current: false
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

Вне скоупа: archetype ECS, универсальный RHI, job system, networking. [[feat-spatial-partition-broadphase|feat: Spatial partition broadphase]] и [[feat-object-pool-fx|feat: Object pool for short-lived FX]] — бэклог до подтверждённого bottleneck (этап 9 roadmap).

Порядок:

- [[chore-ci-windows-linux-and-grey-yard-smoke|chore: CI Windows/Linux and grey_yard smoke]] → [[chore-rat-core-isolation-from-platform-graphics|chore: rat_core isolation from platform graphics]] → [[feat-simulation-session-unified-tick|feat: SimulationSession unified tick]] → [[feat-replay-recording-and-checksum|feat: Replay recording and checksum]] → [[feat-map-document-and-runtime-map|feat: MapDocument and RuntimeMap compile]] → [[jump-button-does-not-always-fire]] → [[ramp-climb-does-not-work]] → [[chore-decompose-editor-app|chore: Decompose EditorApp]] → [[define-asset-folder-load-stub|feat: Asset registry and first load vertical]] → [[interact-button-does-not-always-fire]] → [[cannot-fall-off-ramp]] → [[research-entity-model-ecs-vs-scene|research: Entity model ECS vs scene vs hybrid]] → [[feat-render-world-packets|feat: RenderWorld packets and instrumentation]] → [[chore-native-window-platform-adapters|chore: NativeWindow platform adapters]] → [[feat-input-rebind-gamepad|feat: Input rebind and gamepad]] → [[research-audio-backend-adr|research: Audio backend ADR]] → [[feat-audio-backend-implementation|feat: Audio backend implementation]] → [[chore-engine-frame-metrics|chore: Engine frame metrics]]

## Итог

DoD выполнен 2026-09-01. CI + `rat_core` без GLFW/ImGui/bgfx; один `SimulationSession::tick(InputFrame)` и replay checksum; `MapDocument` → `RuntimeMap`; `EditorApp` — composition root. Карты на `AssetId`; [[ADR-012 Entity model EntityId and ComponentStore]]; renderer ест `RenderWorld` packets. `NativeWindow` / `InputBindings` / [[ADR-013 Audio backend miniaudio]] + miniaudio в editor; метрики в debug snapshot. Play-баги спринта Fixed: jump, interact, ramp climb, walk-off. Вне скоупа остались archetype ECS, RHI, jobs; spatial partition и object pool — бэклог до замера.

## Bugs found

none новых на закрытии. Play follow-up в этом спринте: [[jump-button-does-not-always-fire]], [[interact-button-does-not-always-fire]], [[ramp-climb-does-not-work]], [[cannot-fall-off-ramp]] — Fixed.
