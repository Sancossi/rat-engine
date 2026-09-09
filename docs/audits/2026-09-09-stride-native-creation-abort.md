# Native asset creation: owned abort qualification

Это отдельная техническая граница A1.3 перед import/reimport и остальными resource
операциями. Игровые карты, ресурсы, Core/FSM и пакет A1.2 не менялись. MCP сохраняет
16 ранее принятых tools; пять следующих команд и runtime library этим отчётом не
объявляются выполненными. Контракт: [resource library](../stride-native-resource-library-spec.md).

## Исходный дефект и изменение

В принятом Stride `4d336dac55900c8f0836f04bffb0e985e9145e68` Dispose/Complete
transaction всегда завершает её. Последующий Undo удалял прежнее Redo и оставлял
неудачное создание повторяемым. Исключение CollectionChanged внутри
DirectoryBaseViewModel.AddAsset после InitialUndelete также оставляло asset
зарегистрированным в session, graph container и directory.

Фактический baseline: `build/mcp/creation-failure-20260909-142242-994/result.json`,
лог `build/a13-creation-failure-probe.log`: afterFirstAssetStillRegistered=true,
priorRedoSurvived=false, failedCreationIsRedoable=true, три partial-registration
признака=true. Это воспроизведение, не успешная приёмка.

Source commit `fddf82aa16f547b037a246bcf5cf518a35ccdeee` в собственном
[fork](https://github.com/Sancossi/stride) сохраняет upstream baseline и добавляет:

- `ITransactionStack.AbortTransaction`, `IUndoRedoService.AbortTransaction` и
  событие Aborted. Rollback выполняется до снятия transaction со стека, без
  регистрации failed операции в history и без удаления прежнего Redo.
- Abort допустим только для текущей unshared transaction с default flags.
  Обычный child поддержан; родитель остаётся активным и его completion task не
  завершается раньше времени. Noncurrent/shared/KeepParentsAlive отклоняются.
- `PackageViewModel.CreateAssetsAtomic`: явная directory, prevalidation batch
  IDs/names, собственная root transaction, только принадлежащий вызову список
  созданных assets. При исключении constructor или Initialize откатывается child;
  после abort освобождаются его session/directory/package registration и graph.
  Файлы не удаляются, API произвольного release существующих assets нет.
- DirtiableManager пересчитывает dirty после abort без включения failed операций
  в сохранённый snapshot. Ошибка Aborted subscriber после rollback всё равно
  оставляет IsAborted=true и выполняет cleanup. Ошибка самого rollback оставляет
  poisoned transaction: Complete/new edits/native Save запрещены, требуется reload.

MCP слушает Aborted для монотонной revision и отклоняет изменения при
HasFailedTransaction; status остаётся busy. Ошибка после начала commit может
увеличить revision, даже когда graph/disk/dirty/history восстановлены. Это отличается
от preflight rejection без изменений. Не утверждается общая атомарность любого
произвольного editor/plugin callback: неожиданная ошибка rollback явно диагностируется
как неопределённое состояние, а не успех.

## Проверки

Все пути ниже относительно `C:/5_gamedev/rat-engine`, source относительно
`C:/5_gamedev/stride`. Fixture использует собственную временную директорию,
test-only startup hook и реальные native методы; production eval tools отсутствуют.

| Проверка | Результат и доказательство |
|---|---|
| Core.Design полный suite | 656 passed, 0 failed/skip, 2m21s; `build/a13-abort-stack-tests.log` |
| Новые abort scenarios отдельно | 4 passed: prior Undo/Redo; nested/shared/flags; throwing subscriber/reentry; poisoned rollback. `build/a13-abort-tests/abort.trx` |
| Scoped Release editor build | exit0, 27.34s, 0 errors; `build/a13-abort-editor-build-final.log`. Прямой build с `StrideSkipAutoPack=true`, не canonical stage wrapper |
| Реальные native insertion failures | `build/mcp/creation-failure-20260909-145307-103/result.json`: error после первого добавления; constructor error второго; дополнительно throwing Aborted subscriber. Во всех трёх сохранены graph/disk/dirty/точная предыдущая Undo+Redo history; failed IDs нигде не зарегистрированы |
| Success после failures, новый editor | Там же `reopen.json`: create→Undo→Redo→Save, fresh process загрузил тот же `d4de84e9-283b-4466-8a6e-2b3c17c3d662` и Width73; затем injected rollback failure блокирует native Save/MCP mutation и оставляет busy. Owned PID5208/60820 закрыты |
| Старый resource/API gate | `build/mcp/resource-api-20260909-145409-014/result.json`: 101 calls, 16 tools, 13 ожидаемых ошибок, 0 captures; native two-asset success/mixed batch, single Undo, lease/save/close/terminal guards прошли |
| Старый session gate | `build/mcp/session-state-20260909-145455-142/result.json`: native Save/Close, queued lifecycle guards, disk/dirty/Undo прошли |
| Failed readiness/ownership | `build/mcp/resource-api-20260909-145538-339/result.json`: собственный editor завершён, другой sentinel не затронут, временные assets/resources сохранены в evidence и убраны из production folders; runner exit0 |
| Repository checks | Vault/diff check passed; 6 изменённых PowerShell scripts разобраны parser; Python suite 25 passed, 8.768s |

Команды: `dotnet test sources/core/Stride.Core.Design.Tests/Stride.Core.Design.Tests.csproj -c Release -p:StrideSkipAutoPack=true`;
`scripts/stride/verify-creation-failure.ps1`; `scripts/stride/verify-resource-api.ps1 -McpPython <MCP SDK venv>/Scripts/python.exe`;
`scripts/stride/verify-session-state.ps1`.

Ранние reopen attempts искали bare URL, тогда как native catalog содержит package
prefix. Эти attempts не засчитаны. Финальный reopen адресуется сохранённым native
AssetId и проверяет package, тип и поле. До запуска dirty source корректно отвергался
preflight; обнаруженное маскирование ошибки пустым ownership под StrictMode исправлено
проверкой ContainsKey перед cleanup.

Публикация поверх занятого server DLL первоначально отказала: PID61412/60604 не
останавливались. Source/package cache не очищались. `build-mcp -ServerOutputPath`
позволяет отдельный host; resource/session runners передают каталог своего запуска.
Уже подключённые MCP sessions не переключаются автоматически. NuGet game cohort
и его content hashes не менялись; adapter компилируется против нового editor bin.

Независимое review и canonical full Release stage rebuild выполняются родителем
после этого implementation milestone. Скриншоты, ручное редактирование, фактический
import/decode/audio/runtime-resource acceptance в этой основе не выполнялись.

Parent canonical full Release точного `fddf82aa` прошёл:
`C:/5_gamedev/stride/logs/rat-foundation/20260909-145802-761/result.json`,
exit 0, 1m29.42s, 2143 warnings, 0 errors. Это полный wrapper с упаковкой, а не
повтор предыдущего инкрементального результата с 6 warnings. Среди изменённых
source files предупреждения есть только у прежних не-await вызовов SessionViewModel
(CS4014, строки 726/1606); в новых строках этого среза warnings не обнаружены.
Editor: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`.
Parent также повторил Python 25 tests и vault check — PASS. Сборка на developer PC
не является проверкой чистой машины, ручного F5 или remote CI.

## Исправление после независимого review: poisoned child

Review нашёл один P2 в первоначальном source candidate: CompleteTransaction
извлекал Pop до проверки идентичности. После parent→child→ошибка child rollback
вызов parent.Complete или parent.Dispose выбрасывал исключение, но уже удалял
poisoned child. Чистый Core stack снова позволял CreateTransaction/PushOperation.
Отдельный HasFailedTransaction в native Save/MCP продолжал блокировать их;
обход этих защит не воспроизводился и не заявляется.

Source fix `c0b9065d6e902b45d4d3a5c656318c70df53a6f3` меняет только
TransactionStack и его регрессию. Проверки Count/Peek выполняются до Pop и до
try/finally завершения, поэтому ошибочный parent completion не меняет стек и
не вызывает completion side effects. Тест проверяет оба Complete/Dispose,
сохранность child flag, parent operation и предыдущей history, затем запрет
новых transactions/operations и повторного завершения.

До исправления оба новых случая упали на разрешённом CreateTransaction:
`build/a13-abort-parent-fix/before.trx`, `build/a13-abort-parent-before.log`.
После исправления весь Transactions subset — **32 passed**, включая два новых
случая: `build/a13-abort-parent-fix/after.trx`, `build/a13-abort-parent-after.log`.
Предыдущие 656 Core.Design / 101 MCP calls и canonical Release fddf выше относятся
к foundation prefix; они не переименовываются в результаты нового SHA.

Scoped Release editor на новом pin прошёл: exit0, 52.82s, 2133 warnings,
0 errors; `build/a13-abort-parent-editor.log`, `StrideSkipAutoPack=true`.
Native creation gate повторён на этом pin: runner exit0,
`build/mcp/creation-failure-20260909-150743-953/result.json` и `reopen.json`.
Все три rollback/history/dirty/disk cases, nested completion, create/Undo/Redo/Save
и fresh reopen ID `8d039bd6-2acd-408b-9357-16df71b51c53` прошли; native Save/MCP
poison guards также прошли. Owned PID64796/22780 закрыты, source checkout clean.
Vault/diff checks passed. Final canonical Release и узкое re-review нового pin
остаются отдельными parent gates.

## Acceptance

Повторное независимое read-only review `dd6baea` / source `c0b9065d` — **Approved**.
P2 исправлен, новых блокеров нет. Reviewer проверил код и artifacts; сборки/GUI
самостоятельно не запускал. Parent canonical full Release нового pin прошёл:
`C:/5_gamedev/stride/logs/rat-foundation/20260909-150919-513/result.json`, exit 0,
50.46 s, 6 прежних warnings (5 NU5100, 1 CA1416), 0 errors. Release executable
находится по указанному выше пути. Две посторонние vault EOL правки сохранены.

Bugs found: один review P2 poisoned child исправлен в этом milestone; новых
открытых bugs нет. Закрыта только native creation foundation; MCP import/reimport,
rename/delete/prefab и runtime библиотека остаются следующими срезами A1.3.
