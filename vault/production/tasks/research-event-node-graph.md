---
type: task
area: Engine
status: Done
task_type: Research
sprint: Sprint 8
due:
tags: [task]
---

# research: Event node graph vs bytecode

Intent: как хранить граф и как **компилировать** в pages + `Command` list, не меняя Play interpreter. Сравнить: граф только в Edit (compile on save) vs граф в JSON рядом с командами.

Acceptance: 1 страница в vault: формат, round-trip, что остаётся RM-списком, MVP-ноды (Show Text, Switch, Branch, Wait). Без редактора.

Origin: [[feat: Event node graph authoring]]. Related: [[Event System]], [[ADR-007 Events and maps stored as JSON]]. Взято в [[Sprint 8 — Play feel and authoring]].

---

## Решение

**Граф — только authoring в Edit; Play исполняет существующий линейный список `commands`.** Опциональное поле `graph` на `EventPage` в map JSON; при save/apply Edit **компилирует** граф в `commands[]` и записывает оба поля. `EventRuntime` и loader **не читают** `graph` — источник истины для Play после compile.

Альтернатива **отклонена**: интерпретировать граф в Play (отдельный graph-walker рядом с command-stepper). Два runtime-пути, дублирование лимитов [[ADR-008 Parallel and Autorun runtime limits]], сложнее hot-apply и headless-тесты. Граф = sugar над уже работающим bytecode-like списком ([[Event System]], [[roadmap/grey-box-event-runtime-v1]]).

## Где живёт граф

На уровне **page** (не event, не map):

```json
{
  "trigger": "action",
  "conditions": [],
  "commands": [ "... compiled list ..." ],
  "graph": {
    "nodes": [ { "id": "n1", "kind": "show_text", "params": { "text": "Hello" } } ],
    "edges": [ { "from": "entry", "to": "n1" }, { "from": "n1", "to": "exit" } ]
  }
}
```

- `graph` **optional** — старые карты и ручной RM-список без изменений ([[ADR-007 Events and maps stored as JSON]]).
- `commands` **required для Play** после первого apply с графом; loader валидирует только `commands` (как сегодня, см. `map-event.schema.md`).
- Trigger + page-level `conditions` остаются **вне графа** (как в RM: «стр. 1 — Action, Switch 1 ON»). Граф описывает только тело page после срабатывания.

Специальные pseudo-ноды: `entry` (старт списка команд page) и `exit` (конец page). Ветвления — рёбра `then` / `else` с conditional branch.

## Compile on save / apply

Поток в Edit ([[s2-hot-apply-map-events-json]], [[s2-event-inspector-pages-stub]]):

1. Автор правит граф на canvas **или** список в inspector.
2. **Apply / Save:** если у page есть `graph` и он валиден → `compile(graph) → commands`; иначе `commands` из inspector — как сейчас.
3. Hot-apply в Play получает уже скомпилированный `commands[]` — без изменений в runtime.

Compile — **детерминированный** обход от `entry` по рёбрам; порядок siblings при нескольких исходящих рёбрах задаётся явным `order` на edge или stable sort по target id. Nested `then`/`else` conditional branch → вложенные `Command` с `then_commands` / `else_commands` (существующая форма в `EventPage`).

Ошибки compile (цикл без Wait, unreachable nodes, неизвестный kind, branch без then-edge): **structured error**, карта не стартует / apply отклоняется — как validation map JSON сегодня.

## Round-trip и источник истины

| Слой | Роль |
| --- | --- |
| **Play** | Только `commands[]` — bytecode для [[Event System]] |
| **Edit inspector (list)** | Fallback и power-user UX; если `graph` отсутствует — единственный редактор |
| **Edit canvas** | Редактирует `graph`; list синхронизируется **после compile** (read-only или двусторонний sync только через re-compile) |

**Play truth = compiled `commands`.** Граф — authoring view; не обязан byte-for-byte восстанавливаться из произвольного RM-списка в MVP. Обратная декompilation (commands → graph) — **не в MVP**; ручной список без `graph` продолжает работать.

Если автор удалил `graph` и правит только list — при save пишем только `commands`, canvas для этой page скрыт / «Convert to graph» — follow-up.

## Что остаётся RM-списком (не граф-only)

- Page **trigger** и **conditions** (AND на входе page).
- **Command list** в JSON и в Play — канонический serialized bytecode.
- Inspector **command list** как fallback ([[feat: Event graph editor canvas]] acceptance).
- Все команды вне MVP graph-nodes по-прежнему только через list (Transfer Player, PlaySE, …) до расширения palette.

## MVP-ноды (не полный MZ)

| Graph kind | Maps to `Command.op` | Notes |
| --- | --- | --- |
| Show Text | `show_text` | Один text param |
| Control Switch | `control_switch` | id + value |
| Conditional Branch | `conditional_branch` | condition object + then/else subgraphs → nested commands |
| Wait | `wait` | frames |

Не в MVP canvas: Control Variable, Transfer, Items, Move Route, Label/Jump, Comment-as-node, полный MZ palette. Parallel/autorun semantics не меняются — page trigger задаёт режим, граф — последовательность внутри page.

## Сравнение рассмотренных вариантов

| | Compile in Edit (✓) | Play interprets graph (✗) |
| --- | --- | --- |
| Runtime surface | Один (`EventRuntime` + `Command`) | Два (list + graph VM) |
| Hot-apply | Тот же payload что сегодня | Play должен знать graph schema |
| Headless / golden tests | Compile unit + existing command tests | Дублировать сценарии на graph |
| Git diff | `commands` читаемы; `graph` опционально для review | Graph-only JSON непривычен для RM-авторов |
| Offline / shipped build | Graph можно strip | Graph обязателен в data |

## Follow-up

1. [[feat: Event graph model and compile]] — типы `graph`, serialize/load, compile, golden round-trip, validation errors.
2. [[feat: Event graph editor canvas]] — ImGui/node canvas, compile on apply, inspector fallback.

Epic: [[feat: Event node graph authoring]]. Schema doc update: `docs/schemas/map-event.schema.md` (поле `graph` + node kinds) — в scope compile task, не здесь.

## Resolution

Граф optional на page, Edit компилирует в `commands`; Play не читает `graph`. MVP-ноды: Show Text, Switch, Branch, Wait. Verify: read this card.

## Bugs found

none.
