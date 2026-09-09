---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 19
review: Pending
due:
tags: [task, publication, stride, mcp]
---

# Принятые версии проекта и редактора в main

Intent: по запросу пользователя 2026-09-09 вмержить актуальные проверенные версии проекта и редактора в основные main ветки обоих собственных репозиториев.

Specification: [Source workflow](../../../docs/stride-source-workflow.md); [приёмка MCP](../../../docs/audits/2026-09-09-stride-editor-mcp.md).

Acceptance:

- rat-engine/main сохраняет существующую историю `7129902` и принятую версию `ad733e2`; конфликты разрешены с сохранением актуальных контрактов и проверены независимо.
- В Sancossi/stride создана основная main на принятом integration SHA `88301e861149c48c8b408aac3030b190332d7f97`; master и upstream не переписаны, новый upstream code не подмешан в pinned сборку.
- Remote SHA и default branch проверены; исходные 24 незавершённых A1.2 файла сохранены побайтно. Публикация не закрывает незавершённые игровые срезы.
- Независимое read-only review, vault checks и полная Release сборка редактора сопровождаются точными доказательствами. Изменения только Git topology/docs не требуют повторной игровой квалификации при совпадении исполняемого дерева.

Origin: [[feat-stride-editor-mcp]]; [[publish-editor-game-vault-snapshot]].

## Resolution

Начато 2026-09-09. rat-engine origin/main имеет 5 собственных исторических коммитов относительно принятой ветки; fork Stride master имеет 16 новых upstream commits вне pinned baseline. Слияние проекта готовится в publish/accepted-main; main fork создаётся из принятой интеграции без обновления движка. Parent владеет статусами и финальной публикацией.

## Bugs found

none на старте; проверка слияния продолжается.
