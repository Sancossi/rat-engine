---
type: task
area: Game
status: Done
task_type: Research
sprint: Sprint 9
roadmap: First playable loop
due:
tags: [task]
notion_id: 3ccf3827-36cc-819e-951e-f1f1e0b85306
---

# Collect 5 reference games

Intent: зафиксировать пять референс-игр (движение / арт / диалоги) для first playable и vertical slice. Не клоны — take / don't take под размер rat-engine.

Acceptance: по каждой игре pillar + берём + не берём + wikilinks на ADR/feat. Опечатки сида исправлены (Chrono Cross, Disco Elysium). The Sims — не в пятёрке. Scope сессии — следующая карточка.

Origin: user seed (чат). Взято в [[Sprint 9 — First playable loop]]. Related: [[research: Sims build-mode edit analog]], [[Art Direction]], [[GDD]], [[Event System]].

## Locked five

| Pillar | Game |
| --- | --- |
| Movement | **Metal Gear Solid 3: Snake Eater** |
| Art | **Final Fantasy 6**, **Chrono Cross** |
| Dialog | **Disco Elysium**, **Divinity: Original Sin** (серия: Original Sin / Original Sin 2) |

The Sims уже разобран для Edit ([[research: Sims build-mode edit analog]]) — **не** одна из пяти. Zelda в [[Art Direction]] остаётся optional look-ref, **не** замена FF6 / Chrono Cross.

Vertical slice **использует эти референсы**; конкретный scope сессии — следующая карточка [[Define vertical slice scope]] (сюда не пишем).

## Findings — Metal Gear Solid 3 (movement)

**Pillar:** movement / traversal feel.

**Берём:**

- **Отдельный Climb-рельс, не «идти в лестницу».** Interact лицом к объёму → mount; проход мимо не цепляет. Пока Climb: камера сзади, лестница вверх экрана, вперёд = вверх. Уже в движке: [[feat: MGS3 ladder climb]] (заменил feel [[feat: Ladder toward-climb, jump grab, jump off]]).
- **Свободная ходьба на поле, не tile-step.** Игрок ходит analog/hybrid; авторы снапит к сетке — [[ADR-004 Hybrid map movement and events]]. WASD относительно камеры: [[feat: Camera-aligned walk]].
- **Именованные режимы локомоции** (walk vs climb vs jump), а не один analog-суп — [[feat: Player locomotion FSM]]. Прыжок остаётся своим слайсом ([[feat: Responsive Mario-like jump]]), не CQC MGS.
- **Контекстный Action** на объект (лестница, NPC, дверь) — тот же interact, что у событий ([[Event System]] trigger Action; [[S1: Dialog UI + interact prompt]]).

**Не берём:**

- Стелс как система (камуфляж, фазы тревоги, конусы зрения, CQC).
- Over-the-shoulder / first-person / орбита камеры — ломает [[ADR-003 Ortho pixel-stable camera]] и ortho 3/4 ([[Feat: switchable ortho camera (top-down + 3/4)]]).
- Ползучий prone, плавание, джунгли-traversal, survival-метры (голод, выносливость, раны).
- Спрыгивание с лестницы Jump как основной сход — у нас сход `y_hi`/`y_lo` по рельсу.

## Findings — Final Fantasy 6 (art)

**Pillar:** art / pixel readability.

**Берём:**

- **Читаемые силуэты + ограниченная палитра сцены** (SNES-density, не фото). Базовый texel **32px**, point sample, integer upscale — [[Art Direction]], [[ADR-003 Ortho pixel-stable camera]].
- **Мир «как тайловый», персонаж поверх.** Сетка помогает авторам; игрок не шагает клетку-за-клеткой ([[ADR-004 Hybrid map movement and events]]).
- **Позы NPC / театральность** через mesh-сегменты, не flipbook — [[ADR-009 RE-like segmented character hierarchy]].
- **Один двор / интерьер = один palette lock** для first playable (опера/замок FF6 как урок контраста, не как сеттинг).

**Не берём:**

- Спрайтовых персонажей как пайплайн (решение: mesh + процедурная анимация).
- Mode 7 overworld и perspective follow-cam.
- ATB battle chrome / меню боя — нет battle screen ([[ADR-005 Conflict model exploration plus field action]]).
- Хай-фэнтези магию как визуальный акцент (GDD: средневековье **без магии** + лёгкий стимпанк).
- 16-цветный hardware-лимит SNES как жёсткий cap.

## Findings — Chrono Cross (art)

**Pillar:** art / stylized 3D + pixel.

**Берём:**

- **Живописный pixel albedo на 3D-формах** — ближайший look к «stylized 3D под пиксель-арт» из [[Art Direction]] (у CC: 3D-герои на рисованных фонах; у нас наоборот 3D-мир + pixel textures, тот же контраст).
- **Региональные палитры** (остров = настроение), мягкий dither, акварельная среда без PBR.
- **Фиксированные ракурсы сцены** как дух, не как pre-render: ortho 3/4, без ряби — [[ADR-003 Ortho pixel-stable camera]].
- Сеттинг CC (элементы, параллели) **не** копируем; копируем **плотность цвета и силуэт**.

**Не берём:**

- Pre-rendered backgrounds как пайплайн — ломает [[ADR-006 Edit-in-playmode author loop]] (карта правится в том же exe).
- Element/magic color-coding и 40+ уникальных PC.
- Battle-swirl / отдельный combat art pass.
- **Zelda** как замена этой пары: optional extra в [[Art Direction]], не одна из пяти.

## Findings — Disco Elysium (dialog)

**Pillar:** dialog / text as the quest verb.

**Берём:**

- **Разговор = основной квестовый глагол first playable:** Show Text + Conditional Branch + switches/variables, не скриптовый роман. Уже в модели: [[Event System]] (commands v1, pages + conditions).
- **Выбор, который остаётся флагом** (switch / self-switch / variable), а не «текст и забыл».
- **Разные голоса** (NPC vs «внутренняя реплика») как отдельные строки/страницы, не как OS из 24 навыков.
- Interact prompt + окно текста: [[S1: Dialog UI + interact prompt]]; предметы как ключи к репликам — RM-like список ([[GDD]], [[feat: Inventory list UI]]), Has item в conditions.

**Не берём:**

- 24 навыка + dice checks / Thought Cabinet / роман на длину DE.
- Полный VO и isometric portrait как весь геймплей.
- Подмену field action «игра = только диалог» — MVP всё ещё exploration + light field action ([[ADR-005 Conflict model exploration plus field action]]).
- Политический манифест как обязательный тон (у нас приземлённый стимпанк, [[GDD]]).

## Findings — Divinity: Original Sin (series) (dialog)

**Pillar:** dialog / branching + inventory/world checks.

**Берём:**

- **Ветки, которые смотрят в инвентарь и мир** (есть предмет / switch / variable) — как DOS «use item in conversation». Это уже conditions [[Event System]] (Has item, Switch, Variable), не отдельный dialog engine.
- **Реактивность NPC через pages** (другая страница, если switch ON), не через companion AI Larian.
- **Examine + Talk как Action** на том же событии (prompt уже есть); загадка «скажи / отдай / включи» на 2–3 NPC, не CRPG-партия.
- **Успех/провал как две страницы**, не как бросок кубика в runtime.

**Не берём:**

- 4-player co-op диалоги, Origin character creator, companion romance объёма Larian.
- Turn-based бой + surfaces / стихии, вплетённые в разговор ([[ADR-005 Conflict model exploration plus field action]]).
- Изометрический mouse-CRPG как схема управления (у нас WASD + interact, hybrid move).
- Deep RPG прогрессию (уровни, экип) — [[GDD]] explicit non-goal; инвентарь остаётся RM-списком, не сеткой Divinity.

## Follow-up

- [[Define vertical slice scope]] — vertical slice опирается на эти пять; scope сессии на той карточке, не здесь.
- [[Art direction moodboard pass]] — визуальный lock палитр FF6 / Chrono Cross (уже backlog; не трогаем в этом слайсе).
- [[Art Direction]] — чекбокс конкретных референсов обновлён под эту пятёрку.

## Resolution

Пятёрка: MGS3 (движение), FF6 + Chrono Cross (арт), Disco Elysium + Divinity: Original Sin (диалоги). Take / don't take на этой карточке; The Sims не в списке. Verify: прочитать Findings; чекбокс в [[Art Direction]]. Review: Approved.

## Bugs found

none.
