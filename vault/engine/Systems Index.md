---
type: note
tags: [engine]
---

# Systems Index

Каталог подсистем. Статус: `planned` / `stub` / `working` / `deprecated`.

Сверка с GPP — [[Game Programming Patterns]].

| System | Layer | Status | Owner / notes |
| --- | --- | --- | --- |
| Core runtime loop | Core | working | Init → update → render → shutdown. Fixed 120 Hz accumulator in `EditorApp` |
| Logging | Core | working | `rat::Logger` + sink (file/`stderr`/memory); no locator. Default `rat.log` or `RAT_LOG_PATH` |
| Window + swapchain | Platform / Render | working | GLFW + bgfx present |
| Input actions | Input | working | `InputBindings` table + keyboard/gamepad adapters → `InputButtons` → `InputFrame`; JSON serialize; GLFW/joystick only in `NativeWindow` |
| Asset loader | Assets | planned | |
| Scene / entities | Scene | stub | [[ADR-012 Entity model EntityId and ComponentStore]]: `EntityId` + `ComponentStore` Transform/Renderable/Collider; player/events not migrated |
| Audio | Platform | working | [[ADR-013 Audio backend miniaudio]]: `QueuedAudio` + `AudioSink`; tests swap Recording/Null; `EditorApp` owns `MiniaudioSink` (fallback `LogAudioSink`) and drains once per frame; clips `AssetId` |
| Gameplay notify | Core | stub | GPP Observer: `GameplayNotifyBus` subscribe/post (ItemPicked / DialogShown / Landed); `EventRuntime::set_notify`; no locator |
| Player locomotion | Core | working | GPP State: `locomotion_from` Idle/Walk/Jump/Fall/Climb; `JumpState.climbing` drives Climb. Tick: Climb vs ground/air in `integrate_player_frame_surface`. Jump physics + coyote stay in JumpState. Ladder: Interact-mount rail Climb; while climbing, camera lock (greybox behind player, up = into rungs), W/S = ±Y, A/D ignored, jump ignored. Play walk stays camera-aligned (`C` rotates W when not climbing). Edit: no player WASD. Later clips: [[ADR-009 RE-like segmented character hierarchy]] |
| Collision world | Core | working | `bake_collision_world`: fences, terrain walls, ground walkable boxes, floor slab AABBs (side fences), ramp prisms, ladder volumes. Play: support/ceiling/fall from solids when map passed; ladder Interact rail ([[feat: MGS3 ladder climb]]): no auto-latch, no bounce; top+up steps onto same-tile support; bottom+down to approach. Cylinder vs fences with max_step_up skip; event overlap circle vs AABB. Later: [[feat: Field physics puzzles]], [[feat: Stacked surfaces caves and basements]]. Not ECS |
| Edit history | Core | working | GPP Command: `EditHistory` blockers/events plus height-grid/ramps/edges/floor-slabs/ladders; Ctrl+Z/Y in Edit only; `clear()` on hot-apply |
| Edit gizmos | Input / Scene | planned | [[feat: Mouse viewport map edit]] |
| Agent debug dump | Core | working | `write_debug_snapshot` → `rat-debug.json` (F3); log file `rat.log` |

## Как добавлять систему

1. Строка в этой таблице со статусом `planned`.
2. Задача в [[Tasks]] (Area = Engine).
3. После merge — статус `working` + краткая заметка по API.
