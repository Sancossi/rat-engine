---
type: adr
area: Engine
status: Proposed
decided:
tags: [adr]
---

# ADR-015 Voxel 3D terrain

## Context

Playtest `grey_yard`: мост `(6–8, 5)` — airborne `floor_slabs` (`top_y` 2.0). Сверху ходить можно, снизу пройти можно, **забраться нельзя**. 2.5D-модель даёт либо куб `height_grid` до Y=0 в той же клетке, либо плиту без терейн-подхода. Авторы хотят **многоуровневую карту самим терейном** и **вращаемые рампы** (ориентация клина, не spinning prop).

Это снимает «не воксельный мир» из [[feat: Stacked surfaces caves and basements]]. Продукт не Minecraft-sculpt, не новый renderer, не [[feat: Field physics puzzles]]. Карты остаются JSON ([[ADR-007 Events and maps stored as JSON]]). Клетка мира по-прежнему `tile_size` (1 м).

Представление — отдельный выбор. Пользователь уже зафиксировал **воксели + поворот рампы**; ниже — как хранить.

| Вариант | Суть | JSON / greybox / bake | Вердикт |
| --- | --- | --- | --- |
| **(a) Sparse occupancy 1 м** | Список занятых клеток `(x,y,z)`; воздух не пишется. `solid` = куб 1 м; `ramp` = клин в той же клетке с yaw N/E/S/W | Массив в JSON, diff в git. Greybox = уже существующие кубы/призмы. Bake = `WalkableBox` / `WalkableRamp` в `CollisionWorld`. Два солида в одной XZ с дырой между ними — норма | **Рекомендация** |
| (b) N stacked height-grids | Несколько `ground_y` на слой | Несколько сеток хуже diff’атся, чем sparse-список. Слой всё ещё 2.5D: «пусто» vs «залить до пола слоя» — та же дилемма куб/плита. Рампа остаётся на одном слое. Пещеры и заход на мост требуют ручной стыковки слоёв | Отвергнут: умножает текущую модель, не чинит её |
| (c) MagicaVoxel / mesh import | `.vox` или коллижн-меш как SoT | Ломает JSON-SoT и [[ADR-006 Edit-in-playmode author loop]] (второй редактор, бинарный roundtrip). Клинья рамп не first-class (лестница из кубиков или mesh collider ≈ field physics). Импорт **в** occupancy можно добавить позже | Отвергнут как источник истины |

Короткий spike в прозе: MagicaVoxel JSON-дамп существует, но Edit-in-playmode + события + рампы не живут в `.vox`. Stacked grids эмулируют occupancy ценой N сеток и без 3D-клетки как атома. Оба провала — JSON/greybox/bake авторского цикла, не «воксели медленные». Двор 16×16 при нескольких этажах — сотни занятых клеток, не плотный `16×H×16` массив.

Тонкая плита 0.25 м **не** эквивалентна кубу 1 м: цилиндр 1.6 под декой `top_y` 2.0 проходит, потому что солид `[1.75, 2.0]`; куб `[1, 2]` головой заденет. Occupancy 1 м не подменяет `floor_slabs` один-в-один.

## Decision

**Источник истины терейна (новая схема) — sparse occupancy.** Шаг вокселя = `tile_size`. Хранить только занятые клетки, не dense 3D-массив.

Иллюстративный JSON (схему фиксирует следующая карточка):

```text
occupancy[]: { x, y, z, kind: "solid" | "ramp", yaw?: "north"|"east"|"south"|"west" }
```

- Целые `(x,y,z)` в клетках карты (`x`/`z` как у тайла, включая отрицательные origin). Мир солида: `[x,x+1] × [y,y+1] × [z,z+1]` × `tile_size`.
- Одна запись на клетку. Воздух = отсутствие записи: дыра, подвал, просвет под мостом.
- **`solid`:** полный куб 1 м → существующий `WalkableBox` (верх = support, низ = потолок, бока = стены). **Не** заливать колонку до Y=0.
- **`ramp`:** клин в клетке. `yaw` — направление подъёма, тот же `RampDirection`, что у сегодняшних `ramps[]`. `low_y = y * tile_size`, `high_y = (y+1) * tile_size`. Не animated spin. **Pitch в v1 нет** (четыре yaw закрывают заход на мост; лишняя ось = новые призмы и greybox без выигрыша для двора).
- Не Minecraft: нет ломания/крафта, нет чанков как геймплей, greedy mesh не обязателен (greybox кубов достаточно).

**Совместимость `grey_yard` (schema 4) — dual-read, не freeze и не silent migrate.**

- Loader **обязан** читать schema 1–4 как сейчас: `height_grid`, `ramps`, `floor_slabs`, `edge_barriers`, `ladders`, `indoor_volumes`.
- Следующий schema bump **добавляет** `occupancy[]`. Пока поля нет — карта schema 4 валидна.
- Bake = **union**: legacy solids (`append_ground_boxes` / ramp prisms / `append_floor_slabs` / walls) **плюс** occupancy → те же `WalkableBox` / `WalkableRamp`. Новых типов в `CollisionWorld` нет.
- `grey_yard` остаётся schema 4, пока отдельная карточка не поставит клин к мосту. `floor_slabs` и `ladders` не выкидывать: тонкие деки и рельсы лазания occupancy 1 м не выражает.
- Place cube на `height_grid` (заливка колонки) остаётся legacy. Новый Place voxel пишет occupancy.

Гибрид движения [[ADR-004 Hybrid map movement and events]] не меняется: свободный цилиндр, события на tile/volume.

## Consequences

- [[feat: Voxel occupancy schema and bake]] — schema bump, parse/dump, unit bake двух вокселей в одной XZ без fill-to-Y=0. Кода в этом ADR нет.
- Edit (следующие карточки): Place/Remove voxel по грани или слою Y; Place ramp с yaw от кликнутой грани. Undo через `EditHistory`.
- Greybox: рисовать occupancy теми же кубами/клиньями, что height-grid/рампы; legacy поля — как сейчас.
- `grey_yard`: заход на мост = ramp-воксель к существующим плитам (или к solid на нужном Y), не замена пролёта метровым кубом, пока нужен проход цилиндра 1.6 снизу на `top_y` 2.0.
- Позже: опциональный `kind: "slab"` или импорт MagicaVoxel **в** occupancy. Не field physics, не смена камеры [[ADR-003 Ortho pixel-stable camera]].

Origin: [[research: Voxel 3D terrain ADR]]. Related: [[feat: Voxel 3D terrain and rotating ramps]], [[feat: Stacked surfaces caves and basements]], [[Cannot climb onto grey_yard bridge]], [[Sprint 14 — Voxel 3D terrain]].
