---
type: note
tags: [game]
---

# Art Direction

> **Status:** Working (2026-09-03). Цель — stylized 3D под пиксель-арт, ortho 3/4, без ряби. Первый scene lock: двор `grey_yard` (секция Moodboard).

## Визуальный язык

- **Камера:** orthographic, угол ~3/4 (фиксированный yaw/pitch в MVP)
- **Мир:** 3D меши; читаемые силуэты, ограниченная палитра
- **Персонажи:** **mesh** + **процедурные анимации** (не flipbook sprites как основа)
- **Сеттинг look:** средневековье без магии + лёгкий стимпанк (механизмы, пар, металл)
- **Не цель:** фотореализм, AAA lighting, perspective follow-cam, магический VFX-акцент

## Moodboard

Первый lock — двор **`grey_yard`**. Один двор = одна палитра: не смешивать второй региональный набор на той же карте. Texel **32px**, point sample; это look-lock (металл / латунь / пыль), не каталог мешей.

### Палитра `grey_yard`

| Роль | Hex | Имя |
| --- | --- | --- |
| Ground | `#6B5E4C` | packed dirt |
| Ground dust | `#8F8372` | dust bloom |
| Crate wood | `#7B5534` | weathered crate |
| Brass | `#C6A45E` | workshop brass |
| Soot / iron | `#2A2927` | sooted metal |
| Sky / fog | `#9BA7B2` | overcast haze |
| Rust accent | `#8A4B32` | rusty cog |

Семь слотов: земля и пыль (двор), дерево ящиков, латунь лома, сажа на gantry/железе, холодный туман неба, ржавчина как единственный тёплый акцент квеста.

### Look-refs (FF6 / Chrono Cross)

Take / don't take целиком — [[collect-5-reference-games|Collect 5 reference games]]. Сюда только то, что красится на `grey_yard`:

- **FF6 — берём:** читаемый силуэт пропа/NPC на ограниченной сценной палитре; один интерьер = один lock (урок замка/оперы, не сеттинг).
- **Chrono Cross — берём:** живописный pixel albedo на 3D-формах (у нас 3D-двор + pixel textures); региональный тон + мягкий fog, без PBR.
- **Не берём:** pre-render фоны (ломает Edit в том же exe), SNES-спрайты персонажей как пайплайн, магический VFX.

Следующая карта — новый palette lock, не расширение этой таблицы.

## Pixel-perfect требования

- Рендер во внутренний буфер кратный базовому texelsize, затем integer upscale (или эквивалент без субпиксельного сдвига)
- Камера снапится к pixel/texel grid при движении (запрет «полупиксельного» дрейфа)
- Фильтрация: point sampling для albedo-пиксельарта; никаких случайных anisotropic shimmer
- Отключить / минимизировать temporal AA на пиксель-слое
- Стабильные UV; избегать тонких суб-тексельных деталей на силуэтах

См. [[ADR-003 Ortho pixel-stable camera]].

## Пайплайн ассетов

1. Concept / palette lock
2. Low-poly или tile-chunk меши + pixel textures
3. Import → engine (texel density checklist)
4. In-game pass под ortho 3/4

## Техзаметки

- Базовый texel: **32px**
- Present: integer upscale / letterbox под desktop-разрешения (point sample)
- Naming: `tex_`, `mesh_`, `tile_`
- Source of truth: TBD folder в репо

## Персонажи (структура)

Модульный rig в духе классического **Resident Evil** ([[ADR-009 RE-like segmented character hierarchy]]):

- **Голова**
- **Грудь (torso upper)**
- **Живот (torso lower / abdomen)**
- **Руки** — несколько сегментов (плечо / предплечье / кисть как минимум)
- **Ноги** — аналогично сегментами (для ходьбы/процедурки)

Анимация MVP: **процедурная** на иерархии сегментов (look-at головы, кач груди/живота, IK/циклы ног — по мере внедрения).

Текстуры: pixel albedo 32px-density, point sample.

## Open questions

- [x] 16px vs 32px base → **32**
- [x] Персонажи → **mesh + procedural anim**
- [x] Иерархия тела → **RE-like segments** (голова / грудь / живот / руки сегментами / ноги сегментами)
- [x] Нужна ли ночь/день и как не сломать палитру — нет
- [x] Конкретные референсы → движение **Metal Gear Solid 3**; арт **Final Fantasy 6** + **Chrono Cross**; диалоги **Disco Elysium** + **Divinity: Original Sin** (серия). Zelda — optional look-ref, не замена FF6/CC. Детали: [[collect-5-reference-games|Collect 5 reference games]].
- [x] Точный набор костей/сегментов (нужен ли отдельный neck/hips) — да

## Implementation status (2026-09-07)

The direction above records accepted product choices, not a shipped importer.
AssetRegistry and MemoryAssetLoader exist; mesh/texture import, pixel-stable textured
scene acceptance and the segmented procedural character remain planned.
The accepted neck/hips answer and no day/night choice are retained.
