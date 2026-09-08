# BMad в rat-engine

Установлены **BMad Core 6.12.0 + полный Game Dev Studio v0.7.2**, без BMM и
дополнительных модулей. Это процессы работы с ИИ: они помогают проектировать,
реализовывать и проверять игру, но не заменяют человеческий плейтест.

## Установка и воспроизводимость

Нужны Node.js ≥20.12, npm, Git, Python ≥3.11 и uv в PATH. Все пакеты устанавливаются
внутри проекта; npm/uv могут использовать обычные пользовательские кеши. На этой
машине проверены Node 24.17.0, npm 11.13.0, Python 3.14.4 и uv 0.11.7.
Обычные `sync`, `check` и Python-тесты проекта сохраняют совместимость с Python 3.9;
Python 3.11+ требуется именно для BMad configure/smoke и upstream resolvers.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/setup-bmad.ps1
```

Скрипт работает и для повторной установки. `tools/bmad/package-lock.json` фиксирует
Core вместе с npm-зависимостями и integrity; `tools/bmad/versions.json` фиксирует
GDS tag и commit `2486f5f5f3b8870baa6cee4615a870c0330f441c`. Официальный installer
получает `--pin gds=v0.7.2`, затем проверяется SHA в его manifest. При несовпадении
setup завершается ошибкой. Автообновлений нет; смена lock/tag — отдельная проверяемая
задача. Для установки нужны сеть либо наполненные кеши npm и BMad.

Установленные upstream skills в `.agents/skills/bmad-*` и `.agents/skills/gds-*`,
служебный `_bmad/` и `tools/bmad/node_modules/` игнорируются Git. Собственные
`rat-*` skills остаются в репозитории. Политика из `tools/bmad/team-config.toml` и
`workflow-policy.toml` разворачивается в поддерживаемые team overrides; адаптер
согласует legacy YAML config с TOML. Для устойчивых изменений редактировать эти
исходники, затем запускать `python scripts/bmad_vault.py configure`; прямые правки
сгенерированной конфигурации не сохраняются при setup. Personal overrides могут
быть сохранены upstream, но не должны отменять контракт vault; smoke обнаружит
отклонение основных языковых и маршрутных настроек.

Оригинальные MIT notices сохранены в `licenses/` и копируются в `_bmad/`.
Лицензии npm-зависимостей остаются внутри локального `node_modules`.

## Ежедневная работа

Запускать ИИ из корня репозитория. После установки начать новый сеанс или обновить
каталог skills средствами клиента. Наличие файлов проверено; автоматическая
перезагрузка каталога в уже открытом клиенте не подтверждалась.

| Задача | Реальное имя установленного skill |
|---|---|
| Понять следующий шаг | `bmad-help` |
| Концепция и GDD | `gds-create-game-brief`, `gds-gdd` |
| Нарратив и интерфейс | `gds-create-narrative`, `gds-ux` |
| Архитектура и пригодность к реализации | `gds-game-architecture`, `gds-check-implementation-readiness` |
| Этапы и конкретная карточка | `gds-create-epics-and-stories`, `gds-create-story` |
| Реализация и независимое ревью | `gds-dev-story`, `gds-code-review`, `bmad-review` |
| Спринт и изменение направления | `gds-sprint-planning`, `gds-sprint-status`, `gds-correct-course` |
| Плейтесты и тест-дизайн | `gds-playtest-plan`, `gds-test-design`, `gds-test-review` |

Пример запроса: «Используй gds-gdd для обновления vault/game/GDD.md по утверждённому
плану; гипотезы отметь отдельно». Перед работой skill загружает проектный
`project-context.md`. Принятые ответы не требуют повторного интервью. При ссылке
на отсутствие старого skill использовать документированную адаптацию, не
утверждать его запуск. В этой установке 41 upstream skill, в том числе все GDS
workflows и все стандартные engine support данные; наличие engine support не
выбирает движок для игры.

## Одна очередь и совместимость статусов

Ведущий агент меняет статусы **только в vault**, затем вызывает:

```powershell
python scripts/bmad_vault.py sync
python scripts/bmad_vault.py check
```

Проекция `docs/bmad/generated/sprint-status.yaml` — JSON в синтаксисе YAML 1.2,
с `development_status`, `card_paths`, `vault_metadata` и SHA256 исходников. Она
охватывает лишь текущий спринт. Синтетический `epic-N` группирует этот спринт для
native читателя и не является вторым игровым эпиком. `card_paths` разрешает
сгенерированные story IDs обратно в реальные карточки. Числовой индекс может
измениться при вставке карточки: постоянная идентичность — её canonical path;
не сохранять generated ID в ссылках, зависимостях или командах pickup.

| Vault | Native статус | Сохраняемая семантика |
|---|---|---|
| Task Not started / Bug Open | backlog | Готовность не подтверждена |
| Task In progress / Bug Investigating | in-progress | Работа началась |
| Task In review / Investigating + review In review | review | Нужна независимая проверка |
| Task Blocked | backlog | blocked=true; pickup запрещён |
| Task Done / Bug Fixed | done | completed=true |
| Task Archived / Bug Wont Fix | done | excluded=true, completed=false |
| review Needs fixes | Исходный статус | blocked=true; pickup следующей работы остановлен |

`ready-for-dev` никогда не назначается автоматически. Нативные рекомендации не
разрешают реализацию: ведущий агент проверяет зависимости, приёмку, пользовательский
scope и порядок спринта. `sync` не читает изменения проекции обратно в vault.
`check` ловит изменение карточки, спринта/board, удаление источника и ручную правку
данных проекции. При отсутствии активного спринта выдаётся пустая проекция с
`__NO_ACTIVE_SPRINT__`; native validate, требующий хотя бы одну story, пропускается.

Native workflows адаптируются через policy: вместо записи standalone stories и
статусов они работают с canonical paths. Это проверенная загрузка политики и
совместимых данных, **не доказательство неизменённого end-to-end upstream writer**.

## Проверки и источники

```powershell
python scripts/bmad_vault.py smoke
python scripts/check_vault.py
python -m unittest discover -s scripts/tests
```

См. [evidence](integration-smoke.md). Результат независимого ревью фиксирует ведущий
агент в карточке интеграции после отдельного reviewer; smoke не выставляет approval.

Официальные источники на 2026-09-08:

- [BMad Method 6.12.0](https://github.com/bmad-code-org/BMAD-METHOD/tree/v6.12.0).
- [Game Dev Studio v0.7.2](https://github.com/bmad-code-org/bmad-module-game-dev-studio/tree/v0.7.2).
- [Настройки GDS](https://github.com/bmad-code-org/bmad-module-game-dev-studio/blob/v0.7.2/src/module.yaml).
- Точный CLI проверен через `bmad-method@6.12.0 install --help`, `--list-tools` и
  `--list-options`; реальные workflow инструкции прочитаны из установленного pack.

Обнаруженные upstream несовпадения: README использует прежние `bmgd-*` команды;
CSV skill-manifest хранит пути до развёртывания, которых нет в Codex layout;
GDS module.yaml содержит внутренний module_version 0.7.0 при release tag v0.7.2;
brief ссылается на старые editorial skills. Discovery проверяет фактические имена
в `.agents/skills`, версия берётся из release SHA, редакторские проходы — из
установленного `bmad-review`. Upstream файлы не переписываются.
