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
  сливается в graph. Это текущий `engineCommit`; baseline и `88301e8` сохранены.

Изменены `SessionViewModel`, `AssetSourcesViewModel`, `ImportedAssetViewModel`.
Busy state выставляется до await и снимается в finally. Ошибка acquisition
notification освобождает reservation. Native Save/Close возвращают false с
диагностикой; после Destroy не принимаются graph changes/source hashes. Патч
не обещает общей атомарности произвольных importer plugins.

## Фактическая проверка

Финальная evidence: `build/mcp/resource-api-20260909-134412-394/result.json`,
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
