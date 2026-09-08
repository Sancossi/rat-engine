---
type: note
tags: [research, expedition, engine, digest, verification]
---

# Verification: Godot и Stride

- **Verified, high:** Godot SpriteBase3D/AnimatedSprite3D поддерживает 2D sprite in 3D, billboard mode, depth test, shading и shadows.
  Source: https://docs.godotengine.org/en/4.7/classes/class_spritebase3d.html (Godot Engine, n.d., accessed 2026-09-08).
- **Verified, high:** Stride 4.3 sprites существуют в 3D, Billboard обращён к камере, Ignore depth опционален, sprite-sheet animation документирована.
  Source: https://doc.stride3d.net/4.3/en/manual/sprites/use-sprites.html (Stride, n.d., accessed 2026-09-08).
- **Verified, high:** Godot 4.7.2 предоставляет Windows Standard/.NET binaries и export templates.
  Source: https://godotengine.org/download/archive/4.7.2-stable/ (Godot Engine, 2026-08-18).
- **Verified, high:** GUT 9.7.1 поддерживает Godot 4.7.x и CLI runner; это current third-party testing path.
  Source: https://github.com/bitwes/Gut (GUT project, n.d.).
- **Unverified, medium:** отсутствие встроенного Godot unit-test framework не подтверждено положительным источником.
- **Verified, medium:** Stride документирует Windows Publish и self-contained `win-x64`; независимая страница относится к 4.2, поэтому точные 4.3 runtime details требуют spike.
  Source: https://doc.stride3d.net/4.2/en/manual/files-and-folders/distribute-a-game.html.
- **Unverified, medium:** Stride engine-in-loop xUnit fixed timestep pattern документирован для 4.2, но совместимость с 4.3/4.4 в пределах freshness window не установлена.
  Source: https://doc.stride3d.net/4.2/en/manual/troubleshooting/unit-tests.html.
- **Verified, high:** Godot MIT разрешает proprietary games при сохранении notices и third-party attribution.
  Source: https://github.com/godotengine/godot-docs/blob/master/about/complying_with_licenses.rst.
- **Verified, medium:** официальный Stride 4.4.0-beta6 NuGet package помечен MIT; exact bundled notices требуют audit.
  Source: https://packages.nuget.org/packages/Stride.Core.Assets/4.4.0-beta6 (NuGet Gallery, 2026-09-08).
- **Verified, high:** Godot 4.7.2 — current stable с 4.8 development; Stride stable 4.3.0.2507 с активной 4.4 beta line.

Strongest case for Stride: обычная C#/.NET модель и documented engine-in-loop xUnit могут быть удобнее для automation-first команды, но это преимущество нужно подтвердить на актуальной версии.
