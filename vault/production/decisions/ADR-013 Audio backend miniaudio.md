---
type: adr
area: Engine
status: Accepted
decided: 2026-09-01
tags: [adr]
---

# ADR-013 Audio backend miniaudio

## Context

Геймплей уже постит в GPP Event Queue: `QueuedAudio` + `AudioSink` (null / log / recording) в `rat_core`. `EditorApp` владеет sink и `drain()` раз в кадр. Locator нет и не появится. Сейчас sink только логирует, PlaySE в editor не слышен.

Нужен реальный backend под этот контракт. Windows — primary ([[ADR-001 Language and runtime]]). Linux editor должен иметь путь, даже если позже. Клипы в runtime — `AssetId` (`AssetKind::AudioClip`), не сырые пути. Тесты остаются headless ([[ADR-010 Catch2 unit and headless mechanics tests]]): устройство не открывают.

Сравнение:

| Вариант | Сильные стороны | Цена сейчас |
| --- | --- | --- |
| **miniaudio** | Один C-файл, public domain / MIT-0; WASAPI на Windows; Pulse/ALSA на Linux; декодеры WAV/FLAC/MP3; легко обернуть как `AudioSink` | FetchContent в следующей карточке; устройство только в editor/platform, не в `rat_core` |
| OpenAL Soft | Привычный 3D API, Linux/Windows есть | LGPL (динамическая линковка); отдельный декодер; context/source/buffer тяжелее нужд ortho PlaySE/BGM |
| XAudio2 | Нативный Windows | Нет Linux-пути без второго backend |
| SoLoud / SDL_mixer / FMOD | Готовый микшер или middleware | Лишний слой над miniaudio, второй стек рядом с GLFW, или коммерческая лицензия |
| Оставить null/log | Тесты уже зелёные | PlaySE в editor не слышен; следующая карточка нечем закрыть |

## Decision

**miniaudio** как единственная device-реализация `AudioSink` для editor.

- Контракт не меняется: геймплей постит `PlaySfx` / `PlayMusic` / `Stop` в `QueuedAudio`; sink применяет FIFO на `drain()`.
- Composition root (`EditorApp`) владеет конкретным sink и `QueuedAudio`. Не Service Locator, не синглтон.
- **Подмена в тестах:** `QueuedAudio` принимает `AudioSink&`. `[audio]` / `[playse]` продолжают инжектить `RecordingAudioSink` / `NullAudioSink` / `LogAudioSink`. `rat_tests` не линкует miniaudio и не открывает устройство.
- **Клипы:** runtime identity — `AssetId` (catalog key). `AudioCommand::id` — этот ключ, не путь. JSON `play_se.id` остаётся строкой catalog key; привязка строки → `AssetRegistry` — в [[feat: Audio backend implementation]].
- Device-код живёт вне `rat_core` (editor или `rat_engine`), рядом с другими platform-адаптерами. `rat_core` остаётся без WASAPI/miniaudio.
- Windows: WASAPI через miniaudio. Linux editor: Pulse/ALSA тем же API. Headless CI — recording/null, без device.
- Инициализация устройства падает → fallback на `LogAudioSink` (editor слышит лог, тесты не затронуты).
- FetchContent miniaudio и сам `MiniaudioSink` — следующая карточка, не эта.

Не выбирать OpenAL, пока [[Audio Direction]] не потребует HRTF / 3D sources. Не выбирать XAudio2, пока Linux editor не снят со стола.

## Consequences

- Open question «miniaudio vs OpenAL» закрыт. Реализация: [[feat: Audio backend implementation]].
- Тестовый контракт (`RecordingAudioSink` / `NullAudioSink`) стабилен; audible path не обязан быть в `rat_tests`.
- Очередь, overflow-метрика и смена PlaySE string → `AssetId` поле — scope следующей карточки / [[chore: Engine frame metrics]], не смена backend.
- Смена на OpenAL Soft или middleware = новый ADR + миграция sink; очередь и AssetId остаются.
