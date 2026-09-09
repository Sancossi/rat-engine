---
type: bug
area: Game
status: Fixed
review: Approved
severity: Medium
sprint: Sprint 19
tags: [bug, expedition, companions]
---

# Последняя крыса слишком далеко отстаёт от игрока

## Repro

1. Запустить P1.3 и пройти по двору достаточно далеко для заполнения следа партии.
2. Сравнить расстояния между лидером, первым и последним спутником.
3. Пользователь 2026-09-09 сообщил: «Последний крыса отстает слишком сильно от игрока».

## Expected

Компактная цепочка с равными интервалами: первый спутник в 0.7, последний в 1.4 единицы фактически пройденного пути от лидера. Спутники сохраняют историю высоты/стойки и не срезают углы.

## Actual

`PartyTrail.Spacing=.7f`, `RearDistance=3.2f`: между спутниками 2.5 единицы пути. Это постоянная дистанция, а не недостаточная скорость догоняния.

Specification: [P1.3](../../../docs/stride-session-layered-traversal-spec.md); `games/rat-expedition/Rat.Expedition.Core/PartyTrail.cs`.

## Acceptance

- Given заполненный прямой след, when лидер движется/останавливается, then интервалы равны 0.7, последняя крыса отстаёт на 1.4; проверка воспроизводит прежнее неверное расстояние.
- Given углы, рампа, лестница, смена стойки, pause и portal/recovery, when партия следует за лидером, then остаются корректными 3D путь, история стойки и reset.
- Адаптирован GPU mixed-height сценарий: прежняя расстановка 0/0/1.6 при дистанции 3.2 не становится условием игрового spacing. Проверка локальности скрытия сохраняется реальным достижимым маршрутом/изолированным fixture с объяснением; production геометрия не меняется ради теста.
- Собран новый committed Release ZIP, пройдены Core/executable проверки, независимое read-only ревью и полная Release-сборка редактора; указаны артефакты. Это исправление замечания, не утверждение о завершении всего ручного P1.

Origin: [[expedition-prototype-traversal]].

## Resolution

Исправлено 2026-09-09, runtime commit `d381940c6aaaca8e2e052197c7889f8900c453e5`: расстояния от лидера 0.7/1.4 вместо 0.7/3.2, равные интервалы через `RearDistance=2*Spacing`. Полный 3D след, стойка и reset сохранены; ограничение истории автоматически уменьшилось до 1.9 единицы плюс сегмент. Production геометрия не менялась.

Регрессия на реальной session сначала дала 62 passed / 1 failed с rear=3.2; после изменения — 63 passed. Проверены ходьба, остановка, пауза и прежние рампа/углы/история стойки. Mixed GPU маршрут снимает настоящий срыв до ухода заднего с настила, затем нижнюю партию и возврат cut; прежняя невозможная при новой дистанции расстановка 0/0/1.6 не навязывается игре. Независимое read-only ревью `stride_blind_review`, диапазон `c2bc8d7..d381940`: Approved, замечаний нет.

Итоговая сборка `scripts/stride/build-game.ps1`, exit 0: [ZIP](../../../build/stride-game/20260909-084326-721/rat-expedition-0.1.0-win-x64.zip), [exe](../../../build/stride-game/20260909-084326-721/publish/Rat.Expedition.Windows.exe). Manifest HEAD `9d40b5a4b75f7aed808a10943cd5490b6e03905b`, `gameWorkingTreeDirty=false`, SDK 10.0.300, Stride 4.4.0-dev. SHA256 `4A6046CE20FFD5BC7CA128C950C9BB5FC13382E936D55C61585362D438FF9BBA`. Core 63/63; publish: один CS0162 в generated code, ошибок нет.

`scripts/stride/verify-game.ps1 -PackageZip <этот ZIP>`: exit 0, 29/29 сценариев (16 успешных и 13 ожидаемых отказов). Evidence: `C:/5_gamedev/rat-expedition-validation/20260909-084405-633/verification.json`, отдельные extracted/workingDirectory. Ведущий осмотрел итоговые `layered-1280x720/session-ramp-ascent.png`, `layered-1920x1080/session-ramp-ascent.png`, `mixed-companions/session-mixed-lower.png`; три крысы идут компактнее, высоты и depth сохранены. Это GPU-автоматизация и просмотр кадров, не завершение остальных ручных сценариев P1.

Полный Release редактор: `scripts/stride/build.ps1`, exit 0, 5 upstream NU5100 / 0 ошибок, `C:/5_gamedev/stride/logs/rat-foundation/20260909-084418-536/result.json`. Executable: `C:/5_gamedev/stride/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe`. Upstream чист. Python suite: 22 passed; vault, sprint projection и diff checks прошли при закрытии. A1 и добавление новых спрайтов не начинались.

## Bugs found

none — открытых дефектов исправления не найдено. Результат остальных ручных сценариев P1 пока не получен.
