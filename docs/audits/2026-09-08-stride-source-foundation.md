# Проверка исходной основы Stride — 2026-09-08

Scope: [спецификация](../stride-source-foundation-spec.md),
[workflow](../stride-source-workflow.md). Исполнитель проверил локальный исходный
checkout и Windows x64 Release-сборку Game Studio. Игровой P1 и MCP этим срезом не реализованы.

## Исходники и среда

- Репозиторий: `C:/5_gamedev/stride`, remote `upstream` → `https://github.com/stride3d/stride.git`.
- Ветка: `rat/expedition-foundation`; upstream и фактический HEAD:
  `e2c786a45f69917bf233793f6a097b150e2fe264`.
- Checkout содержит 2602 LFS-файла; `git lfs fsck` завершился `Git LFS fsck OK`.
  Pointer-only строки при проверке `git lfs ls-files` отсутствуют.
- `git status --short` после сборки пуст: runtime и tracked source Stride не изменялись.
  `AGENTS.md` в checkout не обнаружен.
- Windows, .NET SDK `10.0.300`, Git LFS `3.7.1`; установлен Visual Studio 2026 с C++.
  Pinned `global.json`: `10.0.100`, rollForward `latestMinor`, prerelease запрещён.
- Native build path: upstream `dotnet build` / Clang + LLD; LLVM NuGet package
  `Stride.Dependencies.LLVM.Windows/2026.6.11`.

## Выполненные команды и результаты

Копия создана `git clone --origin upstream --no-checkout`, после локального LFS setup
выполнен `git switch -c rat/expedition-foundation <locked SHA>`. Повторный запуск
`powershell -NoProfile -ExecutionPolicy Bypass -File scripts/stride/bootstrap.ps1`
успешен (exit 0), сохраняет HEAD и проверяет LFS.

Первоначальный запуск без ARM64=false завершился exit 1: ARM64 LLD не нашёл
`oldnames.lib` / `MSVCRT.lib`. Артефакты диагностики:
`C:/5_gamedev/stride/logs/rat-foundation/initial-build.log` и `initial-build.binlog`.
Использован поддержанный upstream флаг `StrideNativeWindowsArm64Enabled=false`,
соответствующий требуемому Windows x64. Исходники движка не патчились.

Рабочая команда, используемая [build.ps1](../../scripts/stride/build.ps1):

```powershell
dotnet build sources/editor/Stride.GameStudio/Stride.GameStudio.csproj -c Release -p:StridePlatforms=Windows -p:StrideGraphicsApis=Direct3D11 -p:StrideNativeWindowsArm64Enabled=false -bl:<evidence>/build.binlog
```

| Запуск | Exit | Время MSBuild | Ошибки | Предупреждения |
| --- | --- | --- | --- | --- |
| Первая успешная сборка через wrapper | 0 | 90.00 s | 0 | 2134 |
| Повторная сборка финальным wrapper | 0 | 48.50 s | 0 | 5 |

Первая успешная сборка выполнялась после первоначального restore и частичной компиляции;
это не измерение полной сборки с чистого состояния. Предупреждения происходят из upstream,
включая analyzers/nullability и NU5100 для вложенных native libraries шаблонов.
Во второй сборке остались пять NU5100; они не скрыты и не исправлялись в этом срезе.

Логи первой успешной сборки:
`C:/5_gamedev/stride/logs/rat-foundation/20260908-212415-702/`.
Финальные `build.log`, `build.binlog` и `result.json`:
`C:/5_gamedev/stride/logs/rat-foundation/20260908-212621-553/`.
В manifest записаны реальные HEAD/SDK/аргументы, `exitCode: 0`,
`editorExists: true`, `editorVersion: 4.4.0-dev`.

Исполняемый файл существует:
`C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`.
`LICENSE.md` и `THIRD PARTY.md` присутствуют рядом с executable. Генерируемый
`SharedAssemblyInfo.Generated.cs` содержит PublicVersion `4.4.0`, suffix `-dev`;
исходный sentinel `4.4.65534` не является версией полученной сборки.

## Защитные сценарии

`python -m unittest scripts.tests.test_stride_checkout -v`: 6/6 passed.
Каждый сценарий использует собственную временную локальную fixture без сети:

- Чужой не-Git каталог остаётся с исходным файлом, bootstrap возвращает ненулевой код.
- Неверный upstream остаётся неизменным вместе с HEAD.
- Dirty checkout сохраняет staged и untracked содержимое, индекс и HEAD.
- Собственный коммит поверх baseline допускается и сохраняется.
- Другая ветка отклоняется без переключения.
- Отсутствующий baseline отклоняется без reset.

Проверка descendant использует тот же `Assert-StrideCheckout`, что оба wrappers,
с локальным baseline fixture. Реальный locked baseline проверен повторным bootstrap
и обеими успешными сборками. `python scripts/check_vault.py`: passed.

## Границы свидетельств

После сборки ведущий агент выполнил отдельный запуск редактора в скрытом окне.
Через 10 секунд процесс оставался живым, `Responding=true`, заголовок окна:
`Project selection - Stride Game Studio 4.4.0-dev (.NET 10.0.8)`.
Manifest: `C:/5_gamedev/stride/logs/rat-foundation/startup-smoke/result.json`.
После проверки закрыт именно созданный процесс. Это smoke-проверка запуска до выбора
проекта; интерактивное создание/редактирование сцены и GUI-приёмка не выполнялись.

Исполнитель не запускал GUI, игровую сцену, MCP или автоматические игровые тесты Stride.
Успех компиляции и наличие GameStudio.exe не подтверждают взаимодействие с редактором.
Собственный remote, публичные пакеты или релиз не создавались. Родительский агент
выполняет независимое ревью и закрытие карточки отдельно.

## Исправления после независимого ревью

Проверка выявила три недостатка wrappers и тестов. Относительный `-CheckoutPath`
теперь разрешается относительно текущего расположения PowerShell, включая изменения
через `Push-Location`, а не относительно process working directory .NET. Вход в каталог
сборки использует `-LiteralPath`. Новый сценарий `checkout[rat]` дополнительно воспроизвёл
интерпретацию скобок как wildcard при перенаправлении журнала; запись заменена на
`Out-File -LiteralPath`.

Проверки неверного remote и dirty дерева теперь запускают общий guard с действительным
baseline временной fixture и проверяют конкретное диагностическое сообщение. Они также
сохраняют отдельный вызов bootstrap для проверки подключения guard. Это устраняет прежнюю
возможность ложного успеха теста из-за отсутствия production SHA в fixture.

`python -m unittest scripts.tests.test_stride_checkout -v`: **8/8 passed**.
Тест относительного пути намеренно разводит PowerShell location и .NET working directory.
Тест скобок запускает настоящий build wrapper с временным lock, реальными Git/LFS
проверками и подставным компилятором; проверяет расположение вызова, журнал, manifest,
путь executable и сохранение чистого дерева. Он проверяет wrappers, не компиляцию Stride.

Настоящая повторная сборка исправленным wrapper завершилась exit 0 за 48.75 s:
0 ошибок, 5 прежних upstream NU5100, executable существует, версия `4.4.0-dev`.
Логи и manifest: `C:/5_gamedev/stride/logs/rat-foundation/20260908-213636-378/`.
