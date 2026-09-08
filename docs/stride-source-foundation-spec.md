---
title: Stride upstream source foundation
type: chore
created: 2026-09-08
baseline_commit: 702cae4
context: [docs/bmad/project-context.md, docs/stride-mcp-integration-research.md]
---

# Исходная основа Stride

Статус и исполнение: [карточка](../vault/production/tasks/feat-stride-source-foundation.md). Этот файл конкретизирует уже авторизованный переход пользователя на официальный Stride; отдельный источник статусов не создаётся.

## Intent

**Problem:** Stride выбран, но отсутствует закреплённая локальная исходная основа для разработки и выпуска собственных исправлений. Историческая игра остаётся в C++ репозитории.

**Approach:** Создать отдельную копию upstream, закрепить Git SHA и собрать Game Studio. Добавить минимальные воспроизводимые команды подготовки/сборки и описать порядок ведения собственных патчей. Игровую миграцию P1 реализовать по отдельной адаптированной спецификации, не смешивать её с подготовкой исходной базы.

## Boundaries & Constraints

- Upstream: `https://github.com/stride3d/stride.git`; начальный закреплённый SHA `e2c786a45f69917bf233793f6a097b150e2fe264` из `master`, полученный `git ls-remote` 2026-09-08. Это snapshot разработки, не обещание stable release.
- Целевая копия `C:/5_gamedev/stride`, отдельно от `rat-engine`; remote `upstream`, локальная ветка `rat/expedition-foundation`. Собственный remote добавляется при фактическом размещении fork; публикация не входит в этот срез.
- Сохранить все текущие изменения и историю rat-engine. Не заменять его remote, не переносить каталоги через удаление, не смешивать движок, игру и vault в upstream дереве.
- Использовать Git LFS и закреплённые upstream настройки .NET/MSBuild. На машине обнаружены .NET SDK 10.0.300, Git LFS 3.7.1 и Visual Studio 2026; точные требования проверить по клонированному SHA.
- Не менять runtime Stride без конкретной ошибки, не внедрять AkerMCP или произвольное исполнение C# этим срезом. Достаточно исходной базы и рабочего редактора; первичная игровая сцена, полный P1 и MCP идут последовательно после подготовки.

## I/O & Edge-Case Matrix

| Ситуация | Результат |
| --- | --- |
| Каталог отсутствует | Создать копию, получить закреплённый SHA/LFS, создать локальную ветку |
| Повторный запуск на ожидаемой копии | Проверить remote/HEAD/LFS; сохранить локальные коммиты, не выполнять reset |
| Чужой каталог, неверный remote или dirty дерево | Понятный отказ без перезаписи и удаления |
| Сеть или зависимость недоступна | Ненулевой exit code и диагностический лог; не объявлять редактор собранным |
| Новые локальные коммиты поверх baseline | Сборка проверяет ancestry и записывает реальный HEAD; bootstrap не стирает коммиты |

## Code Map

- `tools/stride/engine.lock.json`: upstream URL, commit, ветка, тип baseline и параметры локального размещения.
- `scripts/stride/bootstrap.ps1`: безопасное получение и проверка копии, без глобальных Git-настроек.
- `scripts/stride/build.ps1`: вызов поддержанного upstream build, проверка prerequisite, логи вне tracked source и exit code.
- `docs/stride-source-workflow.md`: инструкции работы из двух репозиториев, сборка, LFS, обновления и собственные релизы.
- `docs/audits/2026-09-08-stride-source-foundation.md`: фактическая проверка, SHA/SDK, пути артефактов, ограничения.
- `vault/production/decisions/ADR-018 Rat expedition uses Stride.md`, `docs/bmad/project-context.md`: уточнить upstream source workflow, сохранив предыдущие решения.

## Tasks & Acceptance

Исполнитель создаёт lock и минимальные scripts, проверяет их на отдельной копии, собирает Release Game Studio, записывает evidence и коммитит только свои файлы. Внешний Stride checkout читает собственные AGENTS.md, если они есть. Ненужные CI, общий пакетный менеджер и универсальная система плагинов не создаются.

- Given чистая машина с нужными средствами, when пользователь выполняет задокументированную команду bootstrap, then получается копия закреплённого Stride и локальная ветка с видимым upstream.
- Given чужой/dirty каталог, when вызывается bootstrap, then ошибка оставляет файлы и Git-состояние без изменений; воспроизвести эти случаи в временной fixture.
- Given полная копия LFS, when вызывается build, then получен Release Game Studio executable; записать выход команды и фактический путь.
- Given будущая собственная сборка, when читается workflow, then понятны выбор baseline, очередь патчей, локальные packages, сохранение MIT/third-party notices и отдельный namespace версии; не утверждать, что релиз опубликован.

## Verification

Bootstrap и сборка должны проверяться реальным запуском; негативные filesystem сценарии — на временной fixture без destructive cleanup чужих каталогов. Документы проверяются `python scripts/check_vault.py`, карточки синхронизируются ведущим через `python scripts/bmad_vault.py sync` и `check`. Независимый reviewer проверяет scripts и evidence. Ведущий выполняет финальную Release-проверку после этапа. Сборка не считается GUI-проверкой.
