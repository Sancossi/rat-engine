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
