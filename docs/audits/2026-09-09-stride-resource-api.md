# A1.3: первый срез resource API

[Контракт](../stride-native-resource-library-spec.md). Добавлены четыре MCP-команды
и native source-update guard. Игровые Core/FSM, карты и ресурсы не изменены;
библиотека A1.3 этим срезом не завершена.

## Реализованная граница

`asset_list` / `asset_inspect` читают поддержанные native assets выбранного проекта.
`asset_set_property` изменяет allowlisted typed fields через Quantum и native Undo;
`asset_set_reference` связывает assets одного editable package, проверяя native
типы и GUID владельцев. Sprite/material элементы адресуются inspected index с
expected session revision. `packageKey` — относительный путь package, у которого
нет отдельного native GUID. Внешние packages/asset paths и reparse paths отвергаются
при каждом обращении. Source paths не редактируются этими командами.

Сохранены прежние 12 tools, всего 16. Полный allowlist описан в
[README](../../tools/stride-mcp/README.md). Import/reimport, rename/delete и prefab
placement через MCP ещё отсутствуют. Нет eval или произвольного object graph API.

Engine commits в `Sancossi/stride`, ветка `rat/expedition-foundation`:

- `196759b122545b96d7c494fd159501681536b596`: dispatcher-owned asset-operation lease,
  взаимное исключение native Save/Close и source update, защита после await/Destroy.
- `8227ac7402463b14546fc99a169adb14d72f62f0`: partial result с importer errors не
  сливается в graph. Это pin первого среза до review fixes; baseline и `88301e8` сохранены.

Изменены `SessionViewModel`, `AssetSourcesViewModel`, `ImportedAssetViewModel`.
Busy state выставляется до await и снимается в finally. Ошибка acquisition
notification освобождает reservation. Native Save/Close возвращают false с
диагностикой; после Destroy не принимаются graph changes/source hashes. Патч
не обещает общей атомарности произвольных importer plugins.

## Фактическая проверка

Первичная evidence до review fixes: `build/mcp/resource-api-20260909-134412-394/result.json`,
журнал `build/a13-resource-api-final.log`.

```powershell
powershell -ExecutionPolicy Bypass -File scripts/stride/verify-resource-api.ps1 -McpPython C:/5_gamedev/rat-engine-mcp/build/mcp-parent-client/Scripts/python.exe
```

Официальный Python MCP SDK 2.2: **76 calls, 16 tools, 8 ожидаемых ошибок, PASS**.
Проверены независимость двух textures, изменение/Undo/Redo, sheet pivot, material
color, font size, кириллический TextBlock, model→material и entity→model references.
Неверный тип ссылки, отсутствующий ID, stale revision, неполные координаты и JSON
`1e40`, переполняющий native float, отклонены без изменения revision/dirty/Undo.
Scene→UI page и TextBlock→font поддержаны allowlist, но отдельный live roundtrip
этих двух reference forms здесь не выполнен; он остаётся ресурсной квалификации.

Opt-in hook создаёт свои fixtures через native `CreateAsset`. Затем реальный
`ModelViewModel → Sources.UpdateAssetFromSource` использует управляемый test
importer. Во время await прямые native Save/Close и queued MCP mutation отвергнуты;
disk/dirty сохранены. После merge проверены dirty, Undo/Redo и saved file. Throw и
nonnull partial result с logger errors не меняют graph/source hashes. Destroy во
время await предотвращает поздний merge. Acquisition notification exception,
reentry и повторный Dispose проверены. Это реальный native lifecycle/graph с
управляемым backend, не квалификация Assimp mesh import и не MCP reimport tool.

Owned editor PID `648` закрыт после намеренного Destroy. Runner использует Process
и timestamp descriptor своего запуска, проверяет PID/start time/exe, ожидает exit
не более 10 секунд. `saved-fixture-0/1` рядом с result сохраняют native assets и
временные sources; игровых fixtures в Assets не осталось. Rat PNG/Noto скопированы
только для этого теста, исходные лицензии сохранены. **0 viewport captures и input
events**: визуальная или аудио-приёмка здесь не заявляется.

Прежняя native Save/Close qualification также PASS:
`build/mcp/session-state-20260909-134109-639/result.json`,
`build/a13-session-state.log`. Проверены native Save concurrency/disk/dirty/Undo,
Cancel/error Close, Close→Save и queued action после Destroy. Это locked preflight
стандартного `Rat.Expedition.Authoring.sln`; resource run проверяет явный текущий
`Rat.Expedition.sln`.

Перед этим обнаружен унаследованный NU1004: historical Authoring.Windows lock
не содержал A1.2 project edge Authoring→Core. Добавлены только local-project
dependency/entry; NuGet versions/content hashes не изменены. Native Save добавил
существующий Core project и configuration mappings в historical solution; это
изменение сохранено для согласованного повторного открытия.

MCP unit runner PASS: actual junction replacement и traversal/sibling boundary,
queued timeout без поздней mutation, следующий valid request, framing/oversized
input. Команда: `dotnet run --project tools/stride-mcp/Rat.StrideMcp.Tests -c Release -p:RestoreLockedMode=true`.
`python scripts/check_vault.py` и `git diff --check` прошли.

Build source `8227ac7`: `build/a13-editor-source-8227ac.log`, exit 0, 53.54 s,
0 errors, 2134 upstream build/analyzer warnings. Использованы Windows/D3D11,
`StrideNativeWindowsArm64Enabled=false`, `StrideSkipAutoPack=true`. Ранний запуск
без Windows-only native параметров затронул ARM64 linker и не прошёл; исправленный
запуск прошёл. Parent затем выполнил canonical full Release точного `8227ac7`:
exit 0, 68.77 s, 6 warnings (5 NU5100 и 1 CA1416 в неизменённом
`CompilerCrashCapture.cs`), 0 errors. Evidence:
`C:/5_gamedev/stride/logs/rat-foundation/20260909-134647-678/result.json` и соседние
build.log/binlog. Editor:
`C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`.
Независимый review и публикацию выполняет parent. Принятые A1.2 executable 36 здесь
не повторялись: production runtime/resources не изменены.

## Порядок review

Engine commits → [native race fixture](../../tools/stride-mcp/Rat.StrideMcp.Qualification/ResourceQualification.cs)
→ [adapter](../../tools/stride-mcp/Rat.StrideMcp.Adapter/AssetEditing.cs)
→ [MCP scenarios](../../tools/stride-mcp/verify_resources.py)
→ [owned runner](../../scripts/stride/verify-resource-api.ps1).
Следующий срез: остальные bounded API, затем настоящая библиотека, runtime resource
ownership, визуальный/аудио результат и standalone ZIP.

## Исправления трёх P2 после независимого review

Первый native batch regression на `8227ac7` воспроизвёл отказ обновить два assets:
`build/a13-batch-repro.log`, «Native batch did not invoke both importers».
Exclusive per-asset lease отклонял второй параллельный update, а общий LoggerResult
также отбрасывал результат первого. Source fix
`4d336dac55900c8f0836f04bffb0e985e9145e68` сохраняет один lease на весь native
Selected/All batch и одну Undo transaction, выполняет updates последовательно,
использует отдельный logger на каждый asset с копированием сообщений в общий log.
Partial-error guard сохранён. Публичные individual updates всё ещё отвергают reentry.

Финальный исправленный resource run:
`build/mcp/resource-api-20260909-140442-982/result.json`, журнал
`build/a13-resource-fixed.log`: **101 calls, 16 tools, 13 ожидаемых отказов, PASS**.
Actual private native batch entry Selected/All вызван opt-in fixture через reflection;
MCP eval не добавлен. Проверены два успешных assets и failed-first/valid-second,
Save/Close во время batch, один Undo для всего batch, отсутствие leaked busy/transaction.
Только progress dialog подавляется test dialog-service proxy, чтобы проверка не
ожидала ручного закрытия error log. Native импорт/graph/Undo и guards выполняются.

Sound CompressionRatio проверяется до transaction по native range 1..40 из
`sources/engine/Stride.Assets/Media/SoundAsset.cs`; SampleRate должен быть положительным.
Пять actual MCP negative cases (-1/0/41 и negative/zero SampleRate) проверяют неизменность
значений, revision, dirty и Undo. Own silent WAV служит только source fixture для
metadata; наличие слышимого звука не утверждается.

Failed readiness проверен отдельно:
`build/mcp/resource-api-20260909-140534-001/result.json`, журнал
`build/a13-readiness-failure.log`. Команда `verify-resource-api.ps1 -FailedReadiness`
прошла: test hook задержал host Main, deadline 5 s сработал, owned editor PID 58396
закрыт, sentinel PID 45784 оставался жив и затем закрыт отдельно. Два test marker
файла сохранены в `saved-fixture-0/1` и удалены из authoring folders перемещением.
Launcher публикует ownership сразу после StartProcess; readiness находится внутри
cleanup, runner охватывает try/finally и сам launch. Cleanup использует прямой
Process, проверенные start time/exe и bounded exit, не mutable selected descriptor.

Прежний native Save/Close набор повторно PASS на новом source:
`build/mcp/session-state-20260909-140633-032/result.json`,
`build/a13-session-state-fixed.log`. MCP unit, vault и synchronized BMad projection
также PASS. Resource editor PID 59972 и failed-readiness editor закрыты; runtime
игры и следующие asset operations не менялись.

Scoped editor build нового source PASS: `build/a13-editor-batch-build-fixed.log`,
52.41 s, 2133 warnings, 0 errors. Первичная compile-попытка обнаружила неправильный
аргумент LogKey, исправлена до live tests. Ранее описанный canonical full Release
`8227ac7` относится к доисправленному срезу; новый final Release принадлежит parent.

Parent canonical full Release исправленного `4d336dac` прошёл:
`C:/5_gamedev/stride/logs/rat-foundation/20260909-140946-490/result.json`,
exit 0, 50.12 s, 6 прежних warnings (5 NU5100, 1 CA1416), 0 errors.
Editor executable существует по указанному выше Release пути. Это сборка на
developer PC; новый ручной F5, чистая машина и remote CI не заявляются.

## Acceptance

Независимое повторное read-only review `bcd63d6` / source `4d336dac` — **Approved**:
все три P2 исправлены, новых блокеров не найдено. Reviewer проверил код и artifacts,
самостоятельно не запускал сборки или GUI. Parent дополнительно выполнил Python
25 tests, vault check и sync/check BMad projection — PASS; два посторонних EOL
изменения в vault сохранены. NuGet versions/content hashes не менялись.

Закрыт только первый API milestone. Import/reimport как MCP команды, rename/delete,
prefab placement и настоящая runtime библиотека остаются следующими срезами A1.3.
Bugs found: три review P2 исправлены в этом milestone; новых открытых bugs нет.
