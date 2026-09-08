---
type: note
tags: [research, expedition, engine, digest, verification]
---

# Verification: Wicked Engine и O3DE

## Wicked Engine

- **Verified, high:** активный MIT C++ engine/editor; v0.72.113 опубликован 2026-08-24. Источники: https://github.com/turanszkij/WickedEngine и https://github.com/turanszkij/WickedEngine/releases.
- **Verified, high:** поддерживаются Visual Studio/CMake, static engine library, Editor, Windows template и Samples/Tests.
- **Unverified, medium:** отсутствие stock animated world-space billboard. Найдены соседние primitives (`wiSprite`, camera-facing particle quads, impostors), но не editor-authored depth-tested animated sprite actor.
- **Disputed, medium:** authoring/testing целиком слабее Godot/Stride. Editor, Lua, test application и CI есть; не найден именно documented headless gameplay assertion harness.
- **Unverified, medium:** materially larger operational burden. C++ bootstrap и custom sprite integration указывают на риск, но сопоставимых измерений нет.

Strongest case: активный MIT C++ engine с прямым Windows pipeline, компактной архитектурой, editor, Lua и современным renderer подходит разработчику, готовому владеть интеграцией.

## O3DE

- **Disputed with correction, high:** default Apache-2.0, но license позволяет выбрать MIT; bundled third-party components имеют отдельные условия.
  Source: https://github.com/o3de/o3de/blob/development/LICENSE.txt.
- **Verified, high:** Windows pipeline требует Visual Studio workloads, SDK, CMake, минимум 16 GB RAM, рекомендованные 32 GB, около 40 GB installer или 100+ GB source configuration.
  Source: https://docs.o3de.org/docs/welcome-guide/requirements/.
- **Overturned in broad wording, high:** stock Sprite Renderer существует и поддерживает camera-facing alignment/SubUV animation, но это particle/VFX workflow без обычного depth buffer по умолчанию; production 2.5D character component остаётся unverified.
  Source: https://docs.o3de.org/docs/user-guide/visualization/particles/particle-editor/module-renderer/.
- **Verified, high:** CTest, GoogleTest, PyTest, registered project tests, Editor Python, crash monitoring и artifacts документированы.
  Source: https://docs.o3de.org/docs/user-guide/testing/getting-started/.
- **Verified inference, high:** документированный footprint O3DE materially больше Godot/Stride для малого проекта.

Strongest case: наиболее глубокая engineering surface, editor extensibility и first-class automation среди этих двух кандидатов.
