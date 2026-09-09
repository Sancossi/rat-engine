---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 7
due:
tags: [task]
---

# feat: Input rebind and gamepad

Intent: ребинд клавиш и геймпад поверх [[feat-input-action-mapping|feat: Input action mapping]]. В первой карточке ввода этого нет.

Acceptance: те же `InputFrame` actions с другой раскладки и с геймпада; дефолт WASD сохраняется; bindings сериализуются; без окна — тест маппинга. Keyboard/gamepad adapters; в симуляцию уходит только `InputFrame`.

Depends: [[chore-native-window-platform-adapters|chore: NativeWindow platform adapters]]. Next: [[research-audio-backend-adr|research: Audio backend ADR]]. Взято в [[Sprint 7 — Engine architecture]].

Origin: [[feat-input-action-mapping|feat: Input action mapping]] + merged `docs/archive/cpp/architecture-roadmap.md` (2026-09-01).

## Resolution

Таблица `InputBindings` в `rat_core`: те же `InputFrame` с ребинда клавиш и логического геймпада; дефолт WASD+стрелки/Space/E. JSON schema_version 1; `NativeWindow` семплирует связанные GLFW-клавиши и `glfwGetGamepadState`. Симуляция по-прежнему только `InputFrame`. Verify: `.\build\tests\rat_tests.exe "[bindings]"` и `ctest`. Review: Approved.

## Bugs found

none.
