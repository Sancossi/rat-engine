---
type: note
tags: [research, expedition, engine, digest]
---

# Digest: Windows-поставка, тестирование и миграция

Порядок по этому измерению: **Godot > Stride > Wicked Engine > O3DE > id Tech 4**.

- claim: Godot CLI поддерживает headless execution, запуск scene/script, parse-only, fixed-FPS и headless Windows export; editor и export templates разделены.
  source: https://docs.godotengine.org/en/stable/tutorials/editor/command_line_tutorial.html
  publisher: Godot Engine
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: delivery
- claim: Официальная Godot CLI документация подтверждает automation primitives, но не встроенный unit-test framework пользовательского проекта; harness нужно выбрать отдельно.
  source: https://docs.godotengine.org/en/stable/tutorials/editor/command_line_tutorial.html
  publisher: Godot Engine
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: medium
  class: testing
- claim: Stride 4.3 документирует Windows x64 и runtime-зависимости; игра может использовать установленный .NET либо self-contained publish плюс VC++ runtime.
  source: https://doc.stride3d.net/4.3/en/manual/install-and-update/requirements.html
  publisher: Stride project
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: delivery
- claim: Stride позволяет обычные .NET/xUnit тесты, а официальный пример запускает Game с fixed timestep, двигает frames, проверяет scene behavior и завершает процесс.
  source: https://doc.stride3d.net/latest/jp/manual/troubleshooting/unit-tests.html
  publisher: Stride project
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: testing
- claim: Wicked Engine позиционируется как C++ framework или отдельный editor, предоставляет VS/CMake и отдельные Editor, Template, Samples/Tests проекты.
  source: https://github.com/turanszkij/WickedEngine
  publisher: Wicked Engine project
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: burden
- claim: Wicked runtime напрямую связывает приложение с engine API и WISCENE; без заранее выделенных границ уход потребует замены application code и scene content.
  source: https://github.com/turanszkij/WickedEngine
  publisher: Wicked Engine project
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: medium
  class: migration
- claim: O3DE требует широкий Windows toolchain, Git LFS, third-party cache, engine registration и CMake; проект отдельно собирает GameLauncher, Editor и Asset Processor tooling.
  source: https://github.com/o3de/o3de
  publisher: Open 3D Engine
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: burden
- claim: O3DE документирует CTest, GoogleTest, PyTest, project-registered tests и editor tests с crash monitoring и artifacts.
  source: https://docs.o3de.org/docs/user-guide/testing/getting-started/
  publisher: Open 3D Engine
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: testing
- claim: Поддерживаемый id Tech 4 fork dhewm3 предоставляет CMake, Windows x64 dependency binaries, SDL/OpenAL и mod SDK, но исходники не содержат game data, а готовый официальный Windows binary остаётся 32-bit.
  source: https://github.com/dhewm/dhewm3
  publisher: dhewm3 project
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: delivery

Contradictions: Stride requirements упоминают новый .NET runtime, а test snippet — старый target; O3DE README отстаёт от current requirements; dhewm3 имеет x64 build path, но готовые Windows binaries 32-bit.

Leads: одинаковый Godot/Stride spike с clean export, headless logic test, размером пакета и восстановлением asset import; отдельно проверить Stride self-contained win-x64 и headless tests.

Gaps: нет сопоставимых свежих измерений clean-build time, размера релиза, editor RAM и cold import; Wicked/dhewm3 не подтверждают современный CI-oriented gameplay test harness.
