# Проверка интеграции BMad — 2026-09-08

Карточка: [chore-bmad-vault-integration](../../vault/production/tasks/chore-bmad-vault-integration.md).
Контракт: [утверждённый план](../rat-expedition-preproduction-plan.md).

## Что выполнено

Официальный CLI установил Core 6.12.0 и GDS v0.7.2, SHA
`2486f5f5f3b8870baa6cee4615a870c0330f441c`. Проверены все **41** реально
развёрнутых SKILL.md и help catalog, доступные design/story/review/playtest workflows.
34 workflow получили проектный policy через поддерживаемый TOML override.

`powershell -ExecutionPolicy Bypass -File scripts/setup-bmad.ps1` завершился с
кодом 0 в рабочем проекте с уже существующей установкой. Тот же скрипт проверен
в новом временном проекте: код 0; npm lock, pinned GDS, configure и smoke прошли.
Локальный журнал fresh-проверки:
`C:\Users\bogor\AppData\Local\Temp\rat-bmad-fresh-_iilugoh\setup.log`.
Это новая проектная установка с доступными кешами, не тест полностью пустого
сетевого окружения. Временный каталог оставлен как локальное evidence.

Проверены настоящие native `resolve_config.py` и `resolve_customization.py` для
`gds-gdd`, `gds-create-story`, `gds-sprint-status`, `gds-code-review`: проектная
политика присутствует в effective workflow; язык и маршруты совпадают в TOML
и legacy YAML. Прочитаны соответствующие SKILL.md, включая правила выбора
существующей карточки, источников контекста и отдельного ревью.

## Документационный smoke: дизайн → задача → evidence ревью

Это ограниченная проверка адаптированного документального handoff; она не
выдаётся за полный интерактивный запуск или игровой прототип.

| Шаг | Конкретный вход и проверенный результат |
|---|---|
| Дизайн | Из approved preproduction plan взято требование «единственная редактируемая очередь — vault»; оно уточнено в project-context с путями, владельцем статусов и запретом обратной записи |
| Задача | Уже существующая chore-bmad-vault-integration выбрана явно; её Specification указывает на принятый план, Acceptance содержит установку, русский язык, smoke и независимое ревью; дублирующая native story не создана |
| Handoff | Проекция Sprint 16 содержит эту карточку и её canonical path; original task status сохранён; generated story ID не используется как постоянный идентификатор |
| Проверка исполнителя | Сверены требования карточки с установленными модулями, effective policy и результатами проверок ниже; этот отчёт фиксирует evidence для независимого reviewer |
| Независимое ревью | Не выполнялось исполнителем и не имитируется smoke. Ведущий агент организует read-only review и отдельно запишет решение в карточку |

## Результаты проверок

- `python scripts/bmad_vault.py smoke`: PASS — 41 skill, каталог, версии/SHA,
  русский язык, оба вида config, четыре native resolver, MIT notices и проекция.
- `python scripts/check_vault.py`: **Vault checks passed**.
- `python -m unittest discover -s scripts/tests`: **16 tests, OK**. Шесть новых
  сценариев проверяют все task/bug статусы, отдельный bug review, blocked и
  исключённые карточки, ноль/несколько активных спринтов, current-only filtering,
  изменение исходника и ручную правку проекции, отсутствие обратной записи,
  одноимённые task/bug и идемпотентную генерацию.
- Сравнение исходных dirty файлов с байтовым снимком перед работой: **40/40
  сохранены без изменения**, включая незакоммиченный `.cursor` файл.

## Исправленные несовпадения и границы

Первый повторный setup выявил, что installer help перечисляет `--action install`,
но существующая установка принимает только update/quick-update. Скрипт теперь
выбирает `update` при наличии manifest и не передаёт action для fresh install;
оба пути проверены. Upstream resolver возвращает `{workflow: ...}` — smoke читает
реальную форму, а не предполагаемую. `tomllib` загружается только внутри configure,
чтобы обычные Python-тесты проекта не требовали версии выше 3.9.

Оставшиеся особенности upstream перечислены в README: старые aliases и CSV paths,
внутренний module_version, legacy editorial references. Они обходятся проектной
политикой и проверкой реально установленных файлов; upstream pack не патчится.

Нативная запись sprint-status и отдельных story заменена документированным
протоколом vault. Это не тест совместимости неизменённого native writer и не
принудительное ограничение файловых прав ИИ. Протокол обеспечивают AGENTS,
workflow policy, проверки устаревания и независимое ревью.

GUI редактора, gameplay, автоматический reload skills в текущем клиенте и remote
CI не проверялись. Полную Release-сборку редактора при закрытии этапа выполняет
ведущий агент. Выбор движка и продуктовые документы относятся к следующей карточке.
