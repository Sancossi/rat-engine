# Game Studio MCP

Локальный stdio MCP server управляет отдельно запущенным Game Studio через нативный asset Quantum graph. Требуются Windows, .NET 10 и собранная интеграция Stride `c0b9065d6e902b45d4d3a5c656318c70df53a6f3` поверх upstream `e2c786a45f69917bf233793f6a097b150e2fe264`. SHA и fork закреплены в engine lock. Редактирование не использует мышь, клавиатуру или замену файлов сцены.

## Запуск

Если прежний stdio host ещё работает, его DLL нельзя перезаписывать. Для отдельной
проверки `build-mcp.ps1 -ServerOutputPath build/mcp/<run>/server` публикует новый host
в другой каталог; клиенту передают этот executable явно. Resource/session runners
используют собственный каталог запуска. Это не переключает уже подключённые clients.

`verify-creation-failure.ps1` проверяет native owned abort/creation cleanup,
сохранность Undo/Redo/dirty/disk, ошибки конструктора и подписчика, затем Save и
новый запуск с тем же asset ID. Неудачный rollback блокирует native Save и MCP
изменения; status показывает `HasFailedTransaction` и busy. Набор остаётся **16 tools**:
import/reimport, rename/delete и prefab placement ещё не опубликованы этим срезом.
Подробности: [creation abort audit](../../docs/audits/2026-09-09-stride-native-creation-abort.md).

Из корня checkout:

```powershell
powershell -ExecutionPolicy Bypass -File scripts/stride/build-mcp.ps1
powershell -ExecutionPolicy Bypass -File scripts/stride/start-mcp-editor.ps1
```

Второй скрипт сначала выполняет locked Debug build native solution и только при успехе открывает собственный редактор. `-SolutionPath` задаёт другой native solution; по умолчанию используется A1.1 qualification project этого checkout. Существующие окна не подключаются автоматически. Hook включён только для нового процесса и удаляет свои переменные окружения до запуска compiler/game children. Никаких DLL в каталог движка не устанавливается.

После readiness выбранное подключение находится в `build/stride-mcp/connection.json`; timestamp-копия сохраняется в `build/stride-mcp/sessions/`. Server читает descriptor один раз при старте. Перезапуск редактора требует перезапуска MCP server: старый клиент не переключается на новую сессию незаметно.

Команда stdio для любого MCP host:

```text
<checkout>/build/stride-mcp/server/Rat.StrideMcp.Server.exe --connection <checkout>/build/stride-mcp/connection.json
```

Логи идут в stderr; stdout содержит протокол. Официальный C# SDK `2.2.0` согласует версию протокола. Проверены Python SDK `2.2.0` с `2026-07-28` discovery и legacy initialize `2025-11-25`. Подключение Codex выполняется отдельно конфигурацией проекта; изменение config не обновляет набор инструментов уже работающего чата.

Пример локального `.codex/config.toml` (заменить `<checkout>` абсолютным путём; файл исключён из Git):

```toml
[mcp_servers.stride_editor]
command = "<checkout>/build/stride-mcp/server/Rat.StrideMcp.Server.exe"
args = ["--connection", "<checkout>/build/stride-mcp/connection.json"]
cwd = "<checkout>"
startup_timeout_sec = 20
tool_timeout_sec = 30
```

Для вызовов в текущей сессии имеется настоящий MCP CLI `client.py`. Установить `mcp==2.2.0` в отдельный venv, затем:

```powershell
python tools/stride-mcp/client.py --server <absolute-server.exe> --connection <absolute-connection.json> --tool editor_status --output build/mcp/status.json
```

Без `--tool` CLI запрашивает tools/list. Аргументы сложного вызова передаются через `--arguments-file args.json`, чтобы PowerShell не изменил кавычки. Capture сохраняет ImageContent в отдельный PNG рядом с JSON.

## Контракт команд

`editor_status` возвращает PID/project/session, текущую revision, dirty assets и следующие Undo/Redo transaction IDs. `scene_list`, `scene_inspect`, `scene_open`, `scene_close` всегда адресуют native scene ID. `entity_set_property` дополнительно требует native entity/component IDs, имя одного сериализуемого свойства и `expectedRevision`. Transform поддерживает Position/Rotation/Scale; custom properties — явно помеченные DataMember простые значения. Векторы требуют точные X/Y/Z, quaternion также W; неизвестные поля, неполные координаты, неизвестные enum значения и нечисловые значения отклоняются.

Quantum.Update выполняется в именованной Undo transaction на WPF dispatcher. **Undo/Redo и Save имеют scope всей session**, включая другие сцены. `editor_undo`/`editor_redo` требуют expected transaction ID и revision; `save_session` требует revision. Незавершённое ручное поле нативно подтверждается перед проверкой revision, поэтому его правка вызывает конфликт. Обновления native Undo history и asset properties увеличивают revision независимо от клиента. Нужно заново прочитать состояние после конфликта, затем осознанно повторить команду.

Native `IsSaving` блокирует MCP-изменения на всём протяжении Save, включая сериализацию и Undo save point, независимо от инициатора (GUI, F5 или MCP). `IsClosing` охватывает prompt и вложенный Save, снимается при Cancel/ошибке и остаётся терминальным после успешного Close. `IsSessionDisposed` устанавливается до уничтожения сервисов. Эти сигналы проверяются в самом queued callback; lifecycle также отменяет pipe listener. Старого upstream API без этого патча недостаточно.

`IsAssetOperationInProgress` резервирует session на всё время native source update,
включая ожидание importer. Native Save/Close возвращают false с диагностикой, а
MCP-мутации отклоняются до завершения операции. После Destroy importer не сливает
результат в graph и не принимает source hashes; ошибки importer также не принимаются.
Native Selected/All batch держит одну reservation и Undo transaction, последовательно
обновляет assets с раздельными loggers: ошибка одного не отвергает корректный следующий.

Open/Save возвращают operation ID. `editor_operation` различает running и completed/success; начавшийся native Save не отменяется. Очередь UI проверяет cancellation непосредственно перед выполнением. MCP request имеет 12-секундный предел, включая очередь сервера, pipe request — до 10 секунд; connect — 3 секунды, запись ответа — 10 секунд. Входные MCP строки и pipe requests ограничены 64 KiB, отдельное property value — 4096 символами, pipe response — 24 MiB. Уже начавшаяся короткая синхронная транзакция завершается с результатом, а не объявляется отменённой задним числом.

`viewport_capture` возвращает PNG только из backbuffer указанной открытой видимой вкладки. Отрисовка скрытой вкладки может отсутствовать: команда отказывает, desktop fallback отсутствует. `editor_diagnostics` показывает отдельные error/warning counts и последние сообщения native AssetLog, а также capabilities. Это не API управления сборочными заданиями.

Обычная работа не требует `viewport_capture`: `editor_status` → `scene_list` /
`scene_inspect` → `entity_set_property` с прочитанной `expectedRevision` → повторное
чтение результата → `save_session` и проверка operation. Управление использует
структурированные данные, без фотографий viewport, мыши и клавиатуры. Capture
нужен только при отдельной проверке визуального результата или работающего render.
Всего доступны 16 команд: прежние 12 и четыре ограниченные команды ресурсов.
`asset_list` перечисляет поддержанные editable assets выбранного проекта;
`asset_inspect` возвращает native IDs, поля, зависимости и ссылки.
`asset_set_property` изменяет только allowlist: параметры texture/model/sound,
sprite-sheet и отдельного кадра, постоянный diffuse color, размер font и TextBlock.
`asset_set_reference` связывает model→material, entity ModelComponent→model,
UIComponent→page и TextBlock→font в том же editable package. Оно проверяет native
типы и IDs владельцев. Все изменения требуют `expectedRevision` и участвуют в Undo.
Sound CompressionRatio ограничен native диапазоном 1..40, SampleRate должен быть положительным.
Sprite/material элементы адресуются прочитанным `itemIndex` плюс session revision;
UI/entity/component — собственными GUID. `packageKey` — относительный путь package
в выбранной session: у native package нет отдельного сериализуемого GUID.
Внешние packages, asset paths вне корня проекта и пути через reparse points не
доступны для этих команд. Import/reimport, rename/delete, prefab placement и
произвольный material/UI graph через MCP пока не реализованы. Реальный native
reimport ниже проверяет защиту движка, а не наличие новой MCP-команды.

## Воспроизводимость и проверки

Полная пересборка Stride может переупаковать `4.4.0-dev` с другим SHA512 при тех же DLL. Если NU1403 останавливает preflight, не обновлять lock вслепую. `restore-authoring-cohort.ps1 -VerifiedCache <cache>` проверяет исходные nupkg всех locked Stride packages по SHA512 и восстанавливает только isolated game cache. Несовпадающие старые каталоги перемещаются в scoped `build/mcp/cache-cohort-before-*`; глобальные caches не удаляются. В этой квалификации проверенный источник — исходный checkout `C:/5_gamedev/rat-engine/games/rat-expedition/.packages`.

```powershell
dotnet run --project tools/stride-mcp/Rat.StrideMcp.Tests -c Release -p:RestoreLockedMode=true
python tools/stride-mcp/verify_live.py --server <absolute-server.exe> --connection <absolute-connection.json> --output build/mcp/live
powershell -ExecutionPolicy Bypass -File scripts/stride/build-authoring.ps1
```

Live verifier предназначен только для двух собственных qualification scenes. Он действительно изменяет/saves DisplayLabel и Position первой сцены, проверяет вторую, минимизирует только свой editor на время API операций и восстанавливает окно без активации. Не запускать его против пользовательских карт. UI queue regression использует реальный WPF dispatcher с искусственным блокирующим тестовым действием; этого test command нет в MCP. P1 gameplay/verifier не изменены.

`scripts/stride/verify-session-state.ps1` отдельно собирает opt-in qualification assembly и запускает собственный editor с дополнительным test startup hook. Он проверяет прямой native Save, конкурентную queued MCP-правку, disk/dirty/Undo, Cancel/ошибку Close, Close→Save и queued callback после Destroy. Ответы Close prompt подставляет временный dialog-service proxy в тестовой сессии; production services и набор MCP tools не меняются. В конце runner завершает только свой PID после намеренного native Destroy. `-QualificationResult` у launcher предназначен исключительно этому тесту.

`scripts/stride/verify-resource-api.ps1 -McpPython <venv>/Scripts/python.exe`
создаёт временные native fixtures, проверяет четыре resource tools через официальный
Python MCP SDK, затем native source update с управляемым test importer. Проверяются
disk/dirty/Undo/Redo, Save/Close и MCP во время await, ошибка/частичный результат и
Destroy. Test hook не добавляет execute/eval MCP tool. Runner использует Process и
timestamp descriptor собственного запуска; после закрытия сохраняет fixture assets
и sources в папке evidence. Исходные fixture folders должны отсутствовать до запуска.
Это API/lifecycle qualification; импорт mesh настоящим backend, звук и визуальная
библиотека A1.3 ещё требуют отдельной приёмки.

`verify-resource-api.ps1 -FailedReadiness` отдельно проверяет отказ запуска: opt-in
hook задерживает host Main, launcher достигает deadline, закрывает только свой
Process, а runner сохраняет тестовые файлы в evidence и убирает их из authoring.
Второй owned sentinel process остаётся живым. Владение процессом передаётся runner
сразу после StartProcess, до readiness; mutable выбранный descriptor для cleanup
не используется. Публичных MCP-инструментов fault injection нет.

На проверенной Codex CLI `0.153.4` project config подготовлен, но checkout пока не имеет сохранённого trust record, поэтому CLI его не загрузила. Нативный набор MCP tools Codex требует trusted project и перезапуска подключения; рабочий официальный MCP CLI выше доступен уже сейчас. Глобальные настройки доверия и другие MCP connections не изменялись.

См. [инженерный контракт](../../docs/stride-editor-mcp-spec.md), [историю MCP qualification](../../docs/audits/2026-09-09-stride-editor-mcp.md), [NOTICE](NOTICE).
Native Courtyard/Sluice теперь находятся в текущем repository; открыть их можно
через `start-mcp-editor.ps1 -SolutionPath <checkout>/games/rat-expedition/Rat.Expedition.sln`.
Реальные API wall edit/Undo/Redo/Save/reopen и compiled collision parity описаны в
[A1.2 evidence](../../docs/audits/2026-09-09-stride-native-map-authoring.md).
