---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 7
roadmap: Tool / asset pipeline
due:
tags: [task]
notion_id: 3ccf3827-36cc-8160-9b39-fead8faae340
---

# feat: Asset registry and first load vertical

Intent: этап 5 architecture roadmap (бывший Sprint 0 stub «asset folder + load»). `AssetId` / `AssetPath` / `AssetState` / `AssetRegistry` / `AssetLoader`. Source vs compiled vs CPU vs GPU. Состояния Unloaded/Loading/Ready/Failed. Старт: textures, audio clips, mesh/material descriptors. Fallback, debug names, hot reload в editor без FS в gameplay. GPU destroy после границы кадра. Взято в [[Sprint 7 — Engine architecture]].

Acceptance: карты и gameplay ссылаются на стабильные `AssetId`; битый asset не роняет процесс; headless подменяет registry без GPU и файловой системы.

Depends: [[chore-decompose-editor-app|chore: Decompose EditorApp]]. Next: [[research-entity-model-ecs-vs-scene|research: Entity model ECS vs scene vs hybrid]].

Origin: Sprint 0 stub + merged `docs/archive/cpp/architecture-roadmap.md` (2026-09-01).

## Resolution

`AssetId` / `AssetRegistry` / `MemoryAssetLoader` in `rat_core`. Catalog splits source vs compiled vs CPU vs GPU-stub; `resolve()` does not leak paths. Maps bind stable ids; Failed + fallback does not abort; headless swaps the loader without GPU/FS. GPU destroy after `end_frame`. Verify: `.\build\tests\rat_tests.exe "[asset]"` and `ctest`. Review: Approved after compiled-path split.

## Bugs found

none.
