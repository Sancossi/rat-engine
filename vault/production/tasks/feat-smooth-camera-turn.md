---
type: task
area: Engine
status: Done
task_type: Feature
sprint: Sprint 8
due:
tags: [task]
---

# feat: Smooth camera turn

Intent: камера не телепортируется к целевому ракурсу, а **плавно поворачивается** (lerp/slerp взгляда). Сейчас climb lock и смена `CameraMode` (`C`) мгновенно подменяют eye/look-at.

Acceptance: mount/dismount Climb: eye и look-at доезжают до `climb_camera_pose` / выбранного ortho за ~0.2–0.4 с, без рывка. WASD на climb по-прежнему от **целевого** climb shot (не от полуповернутого кадра), иначе «вперёд = вверх» поедет. `C` в Play тоже крутит плавно. Не свободный orbit мышкой (отдельная тема).

Depends: [[climb-camera-locks-on-far-side]] (сначала правильная сторона, потом lerp).
Origin: chat 2026-09-02 (упд: хочу плавный поворот). Related: [[feat-mgs3-ladder-climb|feat: MGS3 ladder climb]], [[feat-camera-aligned-walk|feat: Camera-aligned walk]]. Взято в [[Sprint 8 — Play feel and authoring]].

## Resolution

Greybox lerps eye/focus to climb lock or `CameraMode` over `kCameraTurnSeconds` (0.3 s). Climb WASD uses the **target** `climb_camera_pose`, not the in-flight camera. `C` when not climbing turns smoothly. Verify: Play mount/`C`; `.\build\tests\rat_tests.exe "[unit][camera]"`.

## Bugs found

none.
