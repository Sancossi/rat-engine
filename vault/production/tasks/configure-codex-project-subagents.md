---
type: task
area: Engine
status: Done
task_type: Feature
sprint:
review: Approved
due:
tags: [task, codex, tooling]
---

# Проектные субагенты Codex

Intent: Реализовать утверждённую пользователем настройку субагентов только для rat-engine; разные модели по сложности, делегирование по необходимости.

Specification: Утверждённый план в сессии от 2026-09-10; [официальная документация](https://learn.chatgpt.com/docs/agent-configuration/subagents?surface=app).

Acceptance:

- Роли `rat_explorer` (gpt-5.6-terra/medium), `rat_implementer` (gpt-5.6-sol/high), `rat_reviewer` (gpt-6-astra/high) в `.codex/agents/` с явными инструкциями.
- Локальный `.codex/config.toml`: agents.enabled=true, max_concurrent_threads_per_session=3, interrupt_message=true. Существующий Stride MCP и личные настройки сохранены; образец параметров и инструкция установки находятся в Git, поскольку локальный config.toml исключён из Git.
- Исследователь и ревьюер работают только на чтение, включая MCP; исполнитель наследует разрешения. Один исполнитель одновременно, независимое ревью, статусы и публикация у основного агента; субагенты не делегируют дальше.
- AGENTS.md разрешает полезные независимые исследования и ревью; простые задачи не требуют субагентов.
- Проверены TOML, строгая загрузка конфигурации клиентом, обнаружение ролей и реальные модели/уровни в новом сеансе, короткие задания без изменения файлов для каждой роли.
- Независимое ревью и python scripts/check_vault.py. Несовместимость загрузки или выполнения ролей явно блокирует полную приёмку.

Origin: Прямой запрос пользователя и утверждённый план настройки субагентов от 2026-09-10.

## Resolution

Принято 2026-09-10. Реализация: `d989256`; независимый read-only reviewer `review_subagent_config` на gpt-6-astra/high дал Approved без замечаний после изучения diff и исходных дочерних rollout. Отдельная задача настройки, без запуска или закрытия игрового этапа/спринта.

Настроены три именованные роли, ограничение в три дочерних потока, один исполнитель и делегирование исследований/ревью по необходимости. Инструкции и переносимый образец: [.codex/README.md](../../../.codex/README.md). Локальный config.toml остаётся ignored; прежние значения Stride MCP сохранены.

Для активации проекта добавлена только точная запись `projects.'c:\5_gamedev\rat-engine'.trust_level = "trusted"` в личный config.toml: нативный `config/read` до этого сообщал disabledReason для проектного слоя. Все ранее существовавшие значения личной конфигурации сохранены.

Validation:

- TOML parsing/assertions и `python scripts/check_vault.py` прошли. При добавлении trust основной агент программно сравнил все прежние значения личного TOML до/после и подтвердил их равенство.
- Codex CLI 0.153.4: app-server `--strict-config`, initialize и `config/read` подтвердили активный проектный слой без disabledReason, enabled=true, max_concurrent_threads_per_session=3, interrupt_message=true и наличие stride_editor. Локальный артефакт: `build/codex-subagents/config-validation.json`.
- Новый нативный сеанс `01a08abd-d8eb-75d1-aaf5-adc136889d1b` создал именно три custom agent types без model/effort overrides; все прочитали свои инструкции и AGENTS.md, вернули результат, завершились; CLI exit 0. Модели и effort подтверждены по SQLite metadata и собственному последнему turn_context каждого дочернего rollout, независимо проверенному ревьюером.
- rat_explorer: `01a08abe-1497-74f0-b895-87cbb990412e`, gpt-5.6-terra/medium; rat_implementer: `01a08abe-2c4b-7fb0-8798-b3b0b355105c`, gpt-5.6-sol/high; rat_reviewer: `01a08abe-4360-74f3-8f27-2782e12ada10`, gpt-6-astra/high.
- Артефакты: `build/codex-subagents/runtime-validation.json`, `smoke-events.jsonl`, `smoke-summary.txt` и `smoke-command.json` в том же каталоге. В tool calls только чтение файлов; тест не изменял файлы, карточки и MCP state.

Границы: проверка headless CLI, без заявления GUI/remote CI приёмки. Smoke-сеанс целиком read-only, включая исполнителя; возможность записи исполнителя не проверялась. WebSocket 403 был восстановлен штатным HTTPS fallback, после чего все роли успешно завершились. Новые роли следует использовать в новом сеансе приложения.

## Bugs found

none
