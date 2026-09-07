---
type: note
tags: [engine]
---

# Systems Index

Каталог подсистем. Статус: `planned` / `partial` / `implemented` / `deprecated`.

Сверка с GPP — [[Game Programming Patterns]].

| System | Layer | Status | Owner / notes |
| --- | --- | --- | --- |
| Core runtime loop | Core | implemented | Init → update → render → shutdown. Fixed 120 Hz accumulator in `EditorApp` |
| Logging | Core | implemented | `rat::Logger` + sink (file/`stderr`/memory); no locator. Default `rat.log` or `RAT_LOG_PATH` |
| Window + swapchain | Platform / Render | implemented | GLFW + bgfx present |
| Input actions | Input | implemented | `InputBindings` table + keyboard/gamepad adapters → `InputButtons` → `InputFrame`; JSON serialize; GLFW/joystick only in `NativeWindow` |
| Asset loader | Assets | partial | AssetRegistry + MemoryAssetLoader and stable IDs; disk mesh/texture import and GPU loading pipeline planned |
| Scene / entities | Scene | partial | [[ADR-012 Entity model EntityId and ComponentStore]]: `EntityId` + `ComponentStore` Transform/Renderable/Collider; player/events not migrated |
| Audio | Platform | implemented | [[ADR-013 Audio backend miniaudio]]: `QueuedAudio` + `AudioSink`; tests swap Recording/Null; `EditorApp` owns `MiniaudioSink` (fallback `LogAudioSink`) and drains once per frame; clips `AssetId` |
| Gameplay notify | Core | partial | GPP Observer: `GameplayNotifyBus` subscribe/post (ItemPicked / DialogShown / Landed); `EventRuntime::set_notify`; no locator |
| Player locomotion | Core | implemented | GPP State: `locomotion_from` Idle/Walk/Jump/Fall/Climb; `JumpState.climbing` drives Climb. Tick: Climb vs ground/air in `integrate_player_frame_surface`. Jump physics + coyote stay in JumpState. Ladder: Interact-mount rail Climb; while climbing, camera lock (greybox behind player, up = into rungs), W/S = ±Y, A/D ignored, jump ignored. Play walk stays camera-aligned (`C` rotates W when not climbing). Edit: no player WASD. Later clips: [[ADR-009 RE-like segmented character hierarchy]] |
| Collision world | Core | implemented | `bake_collision_world`: fences, terrain walls, ground walkable boxes, floor slab AABBs (side fences), ramp prisms, ladder volumes. Play: support/ceiling/fall from solids when map passed; ladder Interact rail ([[feat-mgs3-ladder-climb|feat: MGS3 ladder climb]]): no auto-latch, no bounce; top+up steps onto same-tile support; bottom+down to approach. Cylinder vs fences with max_step_up skip; event overlap circle vs AABB. Later: [[feat-field-physics-puzzles|feat: Field physics puzzles]], [[feat-stacked-surfaces-caves-and-basements|feat: Stacked surfaces caves and basements]]. Occupancy bake: [[ADR-015 Voxel 3D terrain]]. Not ECS |
| Voxel occupancy terrain | Core | implemented | [[ADR-015 Voxel 3D terrain]]: sparse 1 m occupancy + yaw wedges; schema 5 `occupancy[]`; bake into existing CollisionWorld boxes/prisms; Edit Place/Remove voxel and Place ramp voxel |
| Edit history | Core | implemented | GPP Command: `EditHistory` blockers/events plus height-grid/ramps/edges/floor-slabs/ladders; Ctrl+Z/Y in Edit only; `clear()` on hot-apply |
| Edit gizmos | Input / Scene | implemented | [[feat-mouse-viewport-map-edit|feat: Mouse viewport map edit]] |
| Agent debug dump | Core | implemented | `write_debug_snapshot` → `rat-debug.json` (F3); log file `rat.log` |

## Как добавлять систему

1. Строка в этой таблице со статусом `planned`.
2. Задача в [[Tasks]] (Area = Engine).
3. После merge — статус `implemented` + краткая заметка по API.

## Stabilization limits

Implemented means an executable path exists, not that acceptance gaps are closed.
[[project-audit-followups]] tracks storage, dirty/undo, replay completeness, event IDs
and unsupported triggers/transfers. EventTouch has no executing path; cross-map
transfer has no loader. Sprint 15 will reject both explicitly. Same-map transfer works.

| System | Layer | Status | Contract |
| --- | --- | --- | --- |
| SimulationSession | Core | implemented | Shared fixed tick for editor and headless input sequence |
| MapDocument / RuntimeMap | Core | implemented | Author content compiles to runtime |
| Event graph | Authoring / Core | implemented | Play executes graph nodes/edges directly; legacy commands migrate on load; command structs supply node effects |
| Replay | Core | partial | Recording/playback exists; format and checksum strengthening in Sprint 15 |
| Game saves | Core / Platform | partial | Existing RATSAVE1; transactional versioned storage in Sprint 15 |
| GUI automation | Editor | planned | Real input scenarios, captures and package acceptance in Sprint 15 |
