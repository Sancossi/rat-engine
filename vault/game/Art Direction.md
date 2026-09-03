---
type: note
tags: [game]
---

# Art Direction

> **Status:** Working (2026-08-31). Цель — stylized 3D под пиксель-арт, ortho 3/4, без ряби.

## Визуальный язык

- **Камера:** orthographic, угол ~3/4 (фиксированный yaw/pitch в MVP)
- **Мир:** 3D меши; читаемые силуэты, ограниченная палитра
- **Персонажи:** **mesh** + **процедурные анимации** (не flipbook sprites как основа)
- **Сеттинг look:** средневековье без магии + лёгкий стимпанк (механизмы, пар, металл)
- **Не цель:** фотореализм, AAA lighting, perspective follow-cam, магический VFX-акцент

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
- [ ] Нужна ли ночь/день и как не сломать палитру — нет
- [x] Конкретные референсы → движение **Metal Gear Solid 3**; арт **Final Fantasy 6** + **Chrono Cross**; диалоги **Disco Elysium** + **Divinity: Original Sin** (серия). Zelda — optional look-ref, не замена FF6/CC. Детали: [[Collect 5 reference games]].
- [ ] Точный набор костей/сегментов (нужен ли отдельный neck/hips) — да
