---
type: bug
area: Engine
status: Fixed
review: Approved
severity: Medium
sprint:
tags: [bug, stride, tooling]
---

# Zed Roslyn не загружает ссылки Stride

Origin: сообщение пользователя о CS0246 после настройки Zed для Stride.

## Repro

1. Открыть `games/rat-expedition` в Zed с C# 1.2.2 и Roslyn 5.12.0-1.26426.8 (`--stdio --autoLoadProjects`).
2. Открыть `Rat.Expedition.Windows/ExpeditionGame.cs`.
3. Сервер сообщает обнаружение трёх проектов, но не завершает их загрузку; файл попадает в отдельный временный проект без ссылок Stride.

## Expected

LSP загружает все три проекта и разрешает типы Stride; CS0246 для существующих ссылок отсутствует.

## Actual

CLI и design-time MSBuild разрешают ссылки, а автоматическая загрузка LSP теряет проекты. Явное уведомление `project/open` загружает их и возвращает hover `namespace Stride` без ошибок компиляции.

## Specification and acceptance

- Добавить единственное решение `games/rat-expedition/Rat.Expedition.slnx`, включающее Core, Core.Tests и Windows с относительными путями. Это обходит дефект автозагрузки отдельных csproj установленного Roslyn.
- Дополнить README короткой инструкцией открытия каталога игры в Zed и перезапуска language server при уже открытом окне.
- Зафиксировать исходную причину, прямую LSP-проверку и границы проверки в `docs/audits/2026-09-09-zed-stride-lsp.md`.
- Проверить автоматическую загрузку решения без `project/open`, hover Stride, отсутствие severity=1 diagnostics, список проектов решения и vault.
- Не менять игровой код, зависимости, установленный сервер и настройки других языков.

## Resolution

Добавлено единое `Rat.Expedition.slnx`; Roslyn автоматически загружает три проекта
через решение. Коммиты реализации и проверки: `6fc3dd5`, `60fdc55`.
Прямые LSP-запросы без `project/open` вернули hover `namespace Stride` и диагностику
без CS0246/severity 1. Контрольный тестовый проект также разрешает Stride без ошибок.
`dotnet sln list`, vault checker и diff check прошли; независимое read-only ревью
Approved без замечаний. [Причина и доказательства](../../../docs/audits/2026-09-09-zed-stride-lsp.md).
После перезапуска C#-сервера журнал Zed подтвердил завершение инициализации проектов.

## Bugs found

Новых дефектов локального обхода: none. Дефект upstream Roslyn обойдён загрузкой решения;
изменение исходников установленного language server не выполнялось.
