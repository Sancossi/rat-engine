# Первый MCP срез для Game Studio

Реализация: [запуск и API](../tools/stride-mcp/README.md), [квалификация](audits/2026-09-09-stride-editor-mcp.md). Статусы остаются только в карточке. Дальнейший A1.2 wall test здесь не засчитывается.

## Suggested Review Order

1. [Startup hook](../tools/stride-mcp/Rat.StrideMcp.Hook/StartupHook.cs), [native plugin/pipe](../tools/stride-mcp/Rat.StrideMcp.Adapter/Bootstrap.cs).
2. [Quantum/revision/operations/capture](../tools/stride-mcp/Rat.StrideMcp.Adapter/EditorBridge.cs), [UI cancellation boundary](../tools/stride-mcp/UiRequestQueue.cs).
3. [MCP tools](../tools/stride-mcp/Rat.StrideMcp.Server/EditorTools.cs), [process-checked client](../tools/stride-mcp/Rat.StrideMcp.Server/BridgeClient.cs).
4. [Live editor checks](../tools/stride-mcp/verify_live.py), [transport checks](../tools/stride-mcp/verify_transport.py), [dispatcher regression](../tools/stride-mcp/Rat.StrideMcp.Tests/Program.cs).
5. [Build](../scripts/stride/build-mcp.ps1), [owned launch/preflight](../scripts/stride/start-mcp-editor.ps1), [exact package-cohort recovery](../scripts/stride/restore-authoring-cohort.ps1).

Основание: пользователь 2026-09-09 попросил заменить координатный ввод управлением через API, затем явно разрешил реализацию. [Карточка](../vault/production/tasks/feat-stride-editor-mcp.md) — источник статуса. [Предварительное исследование](stride-mcp-integration-research.md) сохраняется; этот документ задаёт текущий узкий срез.

## Граница

Локальный MCP сервер предоставляет типизированные команды; адаптер в процессе Game Studio выполняет их на потоке редактора через Quantum/Undo/Save. Кадр viewport читается на соответствующем render/game потоке. Ни координатные клики, ни переписывание `.sdscene` в обход открытого редактора не заменяют API приёмку.

Основная проверяемая версия: Stride `e2c786a45f69917bf233793f6a097b150e2fe264`, Game Studio Release `4.4.0-dev`, .NET 10. Оценить уже изученный AkerMCP `687d9e91c696b2157060bb95715ab92e2b098ffc` для повторного использования; записать конкретные ограничения перед выбором своего узкого адаптера. Не вести новый широкий поиск кандидатов. Заимствования сохраняют Apache-2.0 notices; собственная реализация опирается на подтверждённые pinned API. Сначала вариант без upstream patch; изменение движка допустимо только после конкретного препятствия, отдельным проверенным срезом.

## Команды и корректность

- `status` / discovery: версия, PID, project/session ids, список capabilities, состояния подключений.
- Открыть/перечислить сцены, прочитать иерархию и свойства объекта по native id. Явная scene identity обязательна при нескольких вкладках.
- Изменить transform или доступное сериализуемое custom property через Quantum в одной именованной Undo transaction. Передать expected revision; отвергать устаревшее состояние, включая внешние ручные правки. Ответ возвращает фактическое значение/revision.
- Undo/Redo и Save через сервисы редактора. Если history/session scope шире сцены, явно назвать это в API и защитить expected revision всей затронутой области; не отменять случайную ручную правку.
- Диагностика ошибок и viewport capture с описанием источника/сцены. Capture timeout/неподдерживаемая возможность — явный отказ, без скрытой подмены изображением рабочего стола.

Локальная связь — named pipe, ограниченная текущим пользователем, с проверкой процесса/сессии. MCP transport — stdio; точные SDK/protocol версии и совместимость клиента фиксируются реализацией. Никакого произвольного execute/eval в штатном наборе. Запросы ограничены по размеру/времени. Мутации сериализуются; ожидание UI имеет отмену/проверку актуальности непосредственно перед исполнением, чтобы после таймаута не выполнить неожиданную запись. Установка адаптера обратима и привязана к явному запуску нашего редактора; startup hook не протекает в asset compiler/игру/другие процессы.

## Реальная приёмка

1. Собрать сервер/адаптер, запустить собственный Game Studio с native проектом. Клиент проходит MCP handshake, discovery и настоящие tool calls; stdout сервера содержит только протокол, logs — отдельно.
2. Прочитать две открытые сцены, изменить свойство целевого объекта без фокуса окна, проверить отображение и неизменность второй сцены. Проверить Undo/Redo, save, штатное закрытие/повторное открытие, повторное чтение и compiled runtime value.
3. Проверить неверные scene/entity/session ids, stale revision, недопустимое значение/путь, недоступный адаптер, timeout и конфликт с отдельной ручной/API правкой. Ошибка не меняет документ и не запускает позднюю мутацию.
4. Получить реальный кадр viewport и диагностику; явно указать ограничения рендеринга в фоне. Отдельно проверить user-facing подключение MCP к Codex либо зафиксировать конкретную границу текущей сессии и предоставить работающий локальный MCP client.
5. Использовать мост для возобновления теста стены A1.2 после принятого первого MCP среза: новая видимая позиция и старое/новое столкновение должны совпасть. Изменения A1.2 в основной рабочей копии сохраняются; до независимой приёмки они не считаются закрытыми.

Самостоятельный runtime-мост для движения персонажа, полноценная оркестрация build jobs и вся библиотека ассетов остаются последующими срезами. Этот MCP не объявляется исправлением причин неожиданного закрытия Game Studio. Сборка редактора не заменяет доказательство API работы.
