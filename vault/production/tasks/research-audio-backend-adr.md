---
type: task
area: Engine
status: Done
task_type: Research
sprint: Sprint 7
due:
tags: [task]
---

# research: Audio backend ADR

Intent: выбрать реализацию под интерфейс [[feat-audio-play-queue-stub|feat: Audio play-queue stub]] (miniaudio / OpenAL / иное). Не Service Locator: провайдер по-прежнему владеет `EditorApp`.

Acceptance: ADR Accepted с Windows-primary и способом подмены sink в тестах. Клипы в runtime — `AssetId`, не сырые пути.

Depends: [[feat-input-rebind-gamepad|feat: Input rebind and gamepad]]. Next: [[feat-audio-backend-implementation|feat: Audio backend implementation]]. Взято в [[Sprint 7 — Engine architecture]].

Origin: [[feat-audio-play-queue-stub|feat: Audio play-queue stub]] + merged `docs/architecture-roadmap.md` (2026-09-01).

ADR: [[ADR-013 Audio backend miniaudio]] (Accepted). miniaudio as `AudioSink`; tests keep Recording/Null; clips `AssetId`. Status unchanged until review.

## Resolution

ADR-013: miniaudio (WASAPI на Windows, Pulse/ALSA на Linux) как единственный device-`AudioSink`. `QueuedAudio` и composition root `EditorApp` без locator. Тесты оставляют Recording/Null; `rat_tests` не открывает устройство. Клипы — `AssetId`. FetchContent — [[feat-audio-backend-implementation|feat: Audio backend implementation]]. Verify: read ADR-013. Review: Approved.

## Bugs found

none.
