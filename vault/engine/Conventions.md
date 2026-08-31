---
type: note
tags: [engine]
---

# Conventions

## Git

- `main` — стабильная линия.
- Feature branches: `feat/...`, `fix/...`, `chore/...`.
- PR с кратким why; ссылка на задачу в vault (`vault/production/tasks/...`), если есть.

## Naming

- Публичный API — стабильные имена; breaking changes только через ADR.
- Файлы/модули отражают слой (core, render, …).

## Задачи на движок

- Area = **Engine**, Type = Feature / Bug / Chore / Research.
- Баги runtime — дублировать в [[Bugs]] с Severity.

## Definition of Done (engine)

- [ ] Код собирается
- [ ] Нет известных регрессий на smoke
- [ ] [[Systems Index]] обновлён при новой подсистеме
- [ ] Breaking API → запись в [[Decisions]]
