# MCP Game Studio: квалификация API

Срез выполнен в отдельном `C:/5_gamedev/rat-engine-mcp`, branch `feat/stride-editor-mcp`, baseline `5f2ca37`. Оригинальная незавершённая A1.2 рабочая копия не изменялась. [Контракт](../stride-editor-mcp-spec.md), [инструкция](../../tools/stride-mcp/README.md). Проверки ниже относятся к локальным Windows/.NET 10 и pinned Stride `e2c786a45f69917bf233793f6a097b150e2fe264`, Game Studio Release `4.4.0-dev`.

## Выбор реализации

AkerMCP изучен на `687d9e91c696b2157060bb95715ab92e2b098ffc`. Переиспользованы адаптированные startup-hook registration и asset-side Quantum pattern, с Apache-2.0 [NOTICE](../../tools/stride-mcp/NOTICE) и лицензией. Полный bridge не подходит текущему контракту: выбор первого hierarchy editor, отсутствие expected revision, fire-and-forget Save и desktop fallback не дают нужных гарантий. Полный server/code execution/build orchestration не включены.

Наш adapter использует public `AssetsPlugin.RegisterPlugin`, `IAssetEditorsManager`, `SessionViewModel.AssetNodeContainer`, `IUndoRedoService`, `SaveSession`. Фактически GameStudioWindow вызывает `InitializeSession` (строка 220); одноимённый protected `SessionLoaded` объявлен, но не является вызываемой точкой этого запуска. Reflection ограничена доступом к защищённым Controller/Game для capture, затем вызов выполняется на editor game dispatcher. Backbuffer PNG получен через Stride Graphics API, без OS screenshot или имитации ввода. Движок не патчился.

Стандартный протокол реализует официальный C# SDK `ModelContextProtocol 2.2.0` только в отдельном server process. Наш транспорт в editor — current-user-only pipe с PID/session/project проверками. MCP stdin окружён ограничителем длины строки; собственный JSON-RPC parser не написан. DLL SDK не внедряются в редактор.

## Найденные и исправленные проблемы

Первый запуск нового worktree получил NU1403 и unloadable custom component. Сессию не сохраняли. Committed locks соответствовали архивам исходного game cache, а свежий локальный feed содержал другие SHA512. Независимое сравнение 10 пакетов показало идентичный payload кроме `package/` и `_rels/`: новая упаковка меняла GUID metadata при той же версии. Evidence: `build/mcp-parent-client/package-repack-comparison.json`. Это не изменение engine DLL. Восстановлены 43 Stride archives с проверкой реального SHA512; locks не ослаблены. Launch теперь требует успешный locked native build перед открытием GUI; mutation/save также отказывают при unloadable scene components.

Первый MCP ImageContent был ошибочно заполнен decoded PNG bytes: SDK 2.2.0 Data ожидает UTF8 bytes base64. Исправлено по XML API docs и проверено независимым Python client, который декодирует реальный PNG. Неуспешный `live1` не считается приёмкой. Закрытие одной из сессий пользователь подтвердил как ручное; причины остальных исчезновений не установлены. Исправление crash движка не заявляется.

## Проверенная работа

Финальный собственный editor PID `65868`, descriptor `build/stride-mcp/sessions/20260909-104946-317/connection.json`. Стабильный profile — `build/stride-mcp/connection.json`; активный server хранит собственный неизменяемый snapshot.

- `build/mcp/live5/result.json`: passed=true, 73 MCP calls. Две native сцены открыты; точные scene/entity/component IDs. Custom DisplayLabel → Undo → Redo, transform X `.75 → 0`, SaveSession completed, close/reopen, readback. Вторая сцена полностью совпадает с исходным inspect. Значение первой сцены после финального save: `Edited through MCP: qualified`, position `(0,0,0)`.
- Во время редактирования `IsIconic=true`, editor PID `65868`, foreground PID `18920`: реальный минимизированный/background editor, без global input. Отдельный MCP client внёс API правку; старый revision первого клиента отклонён. Некорректные entity/component/scene IDs, member path, неполные/лишние/lowercase vector fields и неизвестный enum отклонены без изменения revision. Native Undo/dirty доказаны фактическим поведением, не только изменением CLR поля.
- Реальные MCP images: `build/mcp/live5/viewport-8.png` до transform и `viewport-55.png` после. PNG показывают editor grid и procedural box; parent отдельно декодировал и просмотрел `build/mcp-parent-client/viewport-fixed.png`. Последний diagnostics сообщает Error=0, Warning=0; ошибки выделены независимо от verbose messages.
- `build/mcp/transport1/result.json`: passed=true. Официальный Python SDK проверил wrong session/PID и unavailable pipe с ограниченным ожиданием. Raw legacy initialize `2025-11-25`, initialized notification, ping, unknown method `-32601`, missing tool parameters и oversized input проверены отдельно. Независимый parent SDK probe подтвердил 12 tools и negotiation `2026-07-28` (`live-read.json`), legacy `2025-11-25` (`live-legacy-inspect.json`), wrong session (`wrong-session-live-result.json`). Все эти parent paths находятся в `build/mcp-parent-client/`.
- `dotnet run --project tools/stride-mcp/Rat.StrideMcp.Tests -c Release`: реальный WPF dispatcher искусственно занят тестовым действием; queued mutation deadline 50ms истёк до освобождения через 150ms, mutation count остался 0; последующий валидный запрос выполнился. Проверены framing roundtrip, три invalid lengths и oversized SDK input. Это проверка общего production queue helper; тестового block/eval инструмента в MCP нет.
- `scripts/stride/build-authoring.ps1`: `build/stride-authoring/20260909-105116-159/result.json`, exit0. Self-contained executable `publish/Rat.Expedition.Authoring.Windows.exe` из другого cwd загрузил compiled native scene. `runtime/loaded-asset.json` совпадает с MCP по entity GUID, GameId, DisplayLabel и Position; `runtime/native-scene.png` — GPU render. Предыдущий `20260909-104539-066` также подтвердил промежуточный X=.75. Эти локальные preview manifests честно отмечают dirty authoring files до implementation commit.

## Границы

Save/Undo/Redo имеют scope всей native session. Save после старта не отменяется; возвращается operation ID, completion проверяется отдельно. Deadline regression доказывает отмену queued UI mutation, не отмену уже выполняемого native Save. Ручной незавершённый textbox защищён native focus flush + revision guard по исходному API, но этот отдельный интерактивный сценарий здесь не воспроизводился: внешний конфликт квалифицирован вторым реальным MCP client.

Capture требует открытой видимой editor вкладки; hidden/faulted viewport возвращает ошибку, без desktop fallback. Кадры не означают приёмку непрерывного движения или всех GPU/remote desktop режимов. MCP authoring не изменяет gameplay/FSM/camera, не закрывает A1.2 wall parity и не реализует library/runtime/build-job MCP. Codex registration и финальная независимая проверка source editor выполняются parent после этого implementation slice. Статус/closure хранится только в vault card.
