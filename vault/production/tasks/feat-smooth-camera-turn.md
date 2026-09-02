---
type: task
area: Engine
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Smooth camera turn

Intent: камера не телепортируется к целевому ракурсу, а **плавно поворачивается** (lerp/slerp взгляда). Сейчас climb lock и смена `CameraMode` (`C`) мгновенно подменяют eye/look-at.

Acceptance: mount/dismount Climb: eye и look-at доезжают до `climb_camera_pose` / выбранного ortho за ~0.2–0.4 с, без рывка. WASD на climb по-прежнему от **целевого** climb shot (не от полуповернутого кадра), иначе «вперёд = вверх» поедет. `C` в Play тоже крутит плавно. Не свободный orbit мышкой (отдельная тема).

Origin: chat 2026-09-02 (упд: хочу плавный поворот). Related: [[climb-camera-locks-on-far-side]], [[feat: MGS3 ladder climb]], [[feat: Camera-aligned walk]].
