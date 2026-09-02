---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Camera-aligned walk

Intent: в Play WASD следует камере (`camera_relative_move`): W на экране = вперёд по взгляду. Сейчас ходьба world-aligned (W=−Z) из-за [[camera-switch-changes-wasd-world-directions]]. Для MGS3-лестницы «вперёд = вверх» это нужно, иначе лок камеры и ходьба живут в разных осях.

Acceptance: `map_input_frame` с живым eye/focus: W даёт `move` вдоль look XZ. Без камеры (eye над focus) числа совпадают с `world_aligned_move`. Edit не трогаем (там нет player WASD). `C` в Play крутит направление W — так задумано.

Origin: chat 2026-09-02. Prerequisite for [[feat: MGS3 ladder climb]]. Supersedes Play half of [[camera-switch-changes-wasd-world-directions]].

Spec: `docs/superpowers/specs/2026-09-02-camera-aligned-walk-design.md`
Plan: `docs/superpowers/plans/2026-09-02-camera-aligned-walk.md`

## Resolution

Play `map_input_frame` maps WASD through `camera_relative_move` (`climb_move` same). Degenerate camera (no look XZ) keeps `world_aligned_move` numbers. `C` in Play rotates W by design. Verify: `.\build\tests\rat_tests.exe "[unit][input]"`.

## Bugs found

none.
