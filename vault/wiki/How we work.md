---
type: note
tags: [wiki]
---

# How we work

Knowledge and production live in the git vault (`vault/`). Open that folder as an Obsidian vault. Do not write product tasks or docs to Notion.

## Ритуал спринта

1. **Planning** — выбрать Goal спринта, набрать задачи из [[Roadmap]] / backlog (`status: Not started`).
2. **Daily** — короткий статус: blocked / next.
3. **Review** — что shipped, что переносится.
4. **Retro** — 1–3 улучшения процесса.

Текущий спринт: заметка в `production/sprints/` с `current: true`.

## Definition of Done

- Код/контент в репо, задача `status: Done`.
- На карточке заполнены **Resolution** и **Bugs found** (для любой Area / Type).
- Для engine: [[Systems Index]] / ADR при необходимости.
- Для game: [[GDD]] обновлён, если меняется опыт игрока.

## Как править GDD

- Правки в [[GDD]]; крупные сдвиги pillars — задача Research + заметка в [[Decisions]].

## Area routing

- **Engine** — runtime, API, tools движка.
- **Game** — дизайн, контент, геймплей на движке.
- **Production** — процесс, доки, инфра.

## Задачи и баги

Новые идеи из чата агент кладёт в `vault/production/tasks/` или `vault/production/bugs/` (см. шаблоны в `vault/templates/` и Cursor rule capture-user-proposals). Дедуп — поиск по vault, не Notion. Доска задач в Obsidian: [[Task board]].

Баг или новое требование, вскрывшиеся **во время реализации или тестов**, тоже становятся заметкой (не остаются только в чате), с `Origin` / `Follow-up` на исходную карточку и `sprint` текущего спринта.

## Как агент исполняет работу

- Спринт и пачки карточек — **сабагентами** (один implementer на задачу, затем review).
- **До** старта работы карточка `status: In progress` (и коммит). Пока идёт implementer или review — остаётся In progress (на доске может быть две таких). `Done` только после Approved.
- Закрыл карточку текущего спринта — сразу берёт следующую по порядку спринта, без «следующая задача».
- На закрытой карточке — **Resolution** (что сделали / решение) и **Bugs found** (или `none`); отдельные баги — в `production/bugs/` с `Origin`.
- Готовый срез **коммитится сразу**, без ожидания «закоммить».
