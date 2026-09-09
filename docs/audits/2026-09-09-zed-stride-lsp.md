# Zed: загрузка проектов Stride и ложный CS0246

Дата: 2026-09-09. Карточка: [zed-stride-project-loading](../../vault/production/bugs/zed-stride-project-loading.md).

## Причина

В установленном Roslyn Language Server `5.12.0-1.26426.8`, commit
`3aeb96c9ecc56a5ee483558f9e648e33e7bfe756`, обнаружен дефект времени жизни
pooled `ArrayBuilder` при автоматической загрузке отдельных проектов.
[AutoLoadProjectsInitializer.cs](https://github.com/dotnet/roslyn/blob/3aeb96c9ecc56a5ee483558f9e648e33e7bfe756/src/LanguageServer/Microsoft.CodeAnalysis.LanguageServer/HostWorkspace/AutoLoadProjectsInitializer.cs)
захватывает builder в fire-and-forget `Task.Run`, но освобождает его через `Free`
до гарантированного выполнения `ToImmutable` внутри отложенной работы. Очистка builder приводит
к потере обнаруженных проектов. Файл оказывается во временном проекте без ссылок
Stride, хотя CLI и design-time MSBuild эти ссылки разрешают.

## Исходная проверка через LSP

Прямой протокольный запуск установленного сервера с `--stdio --autoLoadProjects`
для `games/rat-expedition` обнаружил три проекта, но не завершил их инициализацию.
Явное уведомление `project/open` успешно загрузило все три проекта. После загрузки
hover в `Rat.Expedition.Windows/ExpeditionGame.cs` вернул `namespace Stride`;
диагностика содержала только информационный `CA1869`, без ошибок компиляции.
Это отделяет дефект автозагрузки от отсутствующих NuGet-пакетов или `using`.

## Локальный обход

`games/rat-expedition/Rat.Expedition.slnx` явно объединяет Core, Core.Tests и Windows
относительными путями. Windows отмечен как `DefaultStartup`. Единственное решение
в корне папки игры направляет автоматическую загрузку сервера по ветке решения,
обходя дефект загрузки отдельных `csproj`. Игровой код, зависимости и установленный
сервер не изменяются.

## Границы проверки

`dotnet sln Rat.Expedition.slnx list` успешно перечислил три ожидаемых проекта;
`python scripts/check_vault.py` завершился с `Vault checks passed`.

Прямые LSP-запросы проверяют работу сервера, но не подтверждают обновление
диагностики в уже открытом окне Zed. Для такого окна требуется перезапустить
language server из C# файла. Проверка автоматической загрузки решения и итоговые
результаты фиксируются отдельно после появления файла решения.
