---
type: moc
tags: [hub]
---

# Rat Engine Hub

Хаб разработки **rat-engine** (движок) и **Rat Project** (продукт на нём). Источник правды — этот vault в git. Notion больше не пишем.

Инженерные артефакты рядом с кодом: [docs/](../docs/) (схемы, acceptance, design specs).

## Как пользоваться

1. Диздок и вики — [[Engine]], [[Rat Project]], [[Wiki]].
2. Работа — [[Task board]] (виды Current sprint / In progress), [[Tasks]], [[Sprints]], [[Roadmap]], [[Bugs]].
3. Архитектурные решения — [[Decisions]].
4. Секцию Rat Project переименуйте, когда появится название продукта.

## Разделы

- [[Engine]] — цели, архитектура, каталог систем
- [[Rat Project]] — GDD, narrative, art, events
- [[Wiki]] — процесс, глоссарий, заметки
- [[Production]] — задачи, спринты, баги, ADR

## Текущий спринт

Смотри заметку спринта с `current: true` (сейчас [[Sprint 4 — Refactoring and AI workflow]]). Агент ищет `current: true` в `production/sprints/`.

Доска: [[Task board]] — виды **Current sprint** (канбан спринта) и **In progress** (карточки в работе). Статус в YAML заметки = колонка на доске.
