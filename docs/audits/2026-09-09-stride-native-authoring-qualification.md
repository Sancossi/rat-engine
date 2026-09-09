# A1.1 — квалификация native GameStudio

Дата: 2026-09-09. [Контракт и воспроизведение](../stride-native-authoring-qualification-spec.md). Проверена минимальная native сцена с собственным сериализуемым компонентом на pinned Stride `e2c786a45f69917bf233793f6a097b150e2fe264`, packages `4.4.0-dev`.

## Реальный GUI цикл

Отдельный GameStudio открыл `Rat.Expedition.Authoring.sln`, показал сцену/зелёную коробку и `Expedition identity`. Через настоящее поле property inspector изменён `DisplayLabel`: `Before GUI edit` → `Edited in Game Studio`, затем Save. Дополнительный цикл `Edited in Game Studio` → `Roundtrip proof` → Undo → Redo → Undo → Save подтвердил обе команды. После полного закрытия и нового запуска редактора поле снова содержит `Edited in Game Studio`, Asset errors = 0. Parent независимо просмотрел reopened screenshot.

В повторно открытом редакторе вызвана штатная команда F5 через её UI Automation InvokePattern. Она собрала и запустила native Windows project без smoke-аргументов. Runtime загрузил compiled сцену, сохранив EntityId `91095954-12d7-4c83-91da-bd5c8ecb8930`, GameId `qualification-box`, DisplayLabel `Edited in Game Studio`, position `(0,0,0)`. Свои editor/launcher процессы после проверки штатно закрыты.

Снимки именно собственного HWND через PrintWindow сохранены в `C:/5_gamedev/rat-engine/build/stride-authoring/gui-qualification/`: `property-before.png`, `saved.png`, `undo-clean.png`, `redo-clean.png`, `property-reopened.png`. `native-f5/loaded-asset.json` и `native-f5/native-scene.png` скопированы из обычного runtime output. Desktop captures с перекрывающим чужим окном не используются. Ввод ограничен PID/foreground собственного окна; upstream UI не патчился.

## Компиляция и parity

Первоначальная сборка и runtime до GUI дали собственную зелёную коробку и исходную метку; файлы в `build/stride-game/authoring-before-gui2/`. Первый compositor с неиспользуемыми UI/Particles features не имел нужного runtime serializer: они удалены из собственного минимального compositor, после чего native compiler/load прошли. Это не квалификация UI/Particles ресурсов.

`powershell -ExecutionPolicy Bypass -File scripts/stride/build-authoring.ps1` завершился с кодом 0. Первый проверочный publish: `build/stride-authoring/20260909-094422-192/`, включая restore/publish logs, result.json и runtime evidence. Он честно отмечает uncommitted authoringWorkingTreeDirty=true. Запуск выполнялся из `%TEMP%`, с native shader database и self-contained runtime. JSON F5 и wrapper побайтово совпали. PNG 1280×720 также совпали: SHA256 `52D769D11D6E066826BD6899F17D82253B3D8ADBD22B866F04555D3671E23887`; изображение просмотрено, зелёная коробка видима. Компонент и native entity сохраняют identity, значение и transform через реальную сериализацию/asset compilation.

## Границы

Это A1.1, не полная приёмка A1: игровые дворы/sluice ещё используют P1 pipeline, Core не менялся. Не проверены collision authoring, prefab identity при clone, весь каталог assets, reimport/rename/delete, чистая машина или remote CI. Проверочный publish не является дистрибутивом A1/P1 ZIP. F5/GUI действия здесь реальные; это не вывод из headless tests. Полную Release сборку редактора и отдельную регрессию P1 на закрытии среза фиксирует parent в карточке.
