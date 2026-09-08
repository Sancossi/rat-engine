---
type: note
tags: [research, expedition, engine, digest]
---

# Digest: соответствие P1 и authoring

Предварительный порядок по этому измерению: **Godot > Stride > Wicked Engine > O3DE > id Tech 4**.

- claim: Godot 4.7 прямо документирует Sprite3D/AnimatedSprite3D в 3D-мире с фиксированной ортографической или перспективной камерой, включая 2D-анимацию, 3D-фон, освещение и тени.
  source: https://docs.godotengine.org/en/4.7/tutorials/3d/introduction_to_3d.html
  publisher: Godot Engine
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: feature
- claim: Godot документирует CharacterBody3D, floor collision, восьминаправленное движение, scene instancing, camera preview и Orthogonal Camera3D; crouch и ladder остаются игровым кодом.
  source: https://docs.godotengine.org/en/latest/getting_started/first_3d_game/03.player_movement_code.html
  publisher: Godot Engine
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: workflow
- claim: Stride 4.3 SpriteComponent размещает спрайты в 3D-сцене, Billboard обращает их к камере, а depth test можно сохранить.
  source: https://doc.stride3d.net/4.3/en/manual/sprites/use-sprites.html
  publisher: Stride project
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: feature
- claim: Stride Bepu CharacterComponent настраивается в Game Studio и расширяется на C#; slopes/steps входят в базовую физику, а crouch, ladder и локальные occluders требуют пользовательских компонентов.
  source: https://doc.stride3d.net/latest/en/manual/physics/characters.html
  publisher: Stride project
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: workflow
- claim: Wicked Engine предоставляет C++ framework/3D editor, Lua, распространённые форматы моделей, WISCENE и пример character controller, но официальная документация не подтверждает готовый world-space animated sprite component уровня Godot/Stride.
  source: https://github.com/turanszkij/WickedEngine
  publisher: Wicked Engine project
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: medium
  class: workflow
- claim: O3DE имеет Camera, PhysX Character Controller, Simple State, UI-on-mesh, Lua и Script Canvas; базовые 3D-возможности достаточны, но stock 2.5D pipeline не подтверждён.
  source: https://www.docs.o3de.org/docs/user-guide/components/reference/
  publisher: Open 3D Engine
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: feature
- claim: O3DE допускает Lua, Script Canvas и C++ Behavior Context, поэтому custom billboard/visibility component возможен ценой дополнительной инфраструктуры.
  source: https://docs.o3de.org/docs/user-guide/scripting/
  publisher: Open 3D Engine
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: workflow
- claim: DarkRadiant 3.9.0 даёт актуальный Windows-capable idTech4 mapping workflow, но не подтверждает ортографический billboard-character runtime.
  source: https://www.darkradiant.net/about.html
  publisher: DarkRadiant / The Dark Mod team
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: workflow
- claim: Официальный Doom 3 BFG source release не содержит game data и документирует устаревший Windows toolchain; id Tech 4 требует выбора и сопровождения современного source port.
  source: https://github.com/id-software/doom-3-bfg
  publisher: id Software
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: compatibility

Contradictions: Stride имеет сильный billboard primitive, но ladder/crouch всё равно требуют расширения controller; DarkRadiant современнее официального id Tech 4 build; Wicked `wiSprite` не доказывает editor-authored depth-tested 3D billboard pipeline.

Leads: spikes для Stride billboard/alpha/orthographic stability, Wicked camera-facing Lua quad, O3DE runtime orthographic camera/world-space sprite.

Gaps: нет измеренной производительности и packaging; для O3DE, Wicked и id Tech 4 не найден официальный готовый animated billboard actor pipeline.
