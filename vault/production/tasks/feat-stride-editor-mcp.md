---
type: task
area: Engine
status: In progress
task_type: Feature
sprint: Sprint 19
review: Needs fixes
due:
tags: [task, stride, mcp, authoring]
---

# Управление Game Studio через MCP

Intent: По запросу пользователя управлять открытым редактором через API вместо координат мыши и клавиатурного ввода, включая чтение сцены, правку свойств и сохранение с Undo/Redo.

Specification: [Первый MCP срез](../../../docs/stride-editor-mcp-spec.md); [предварительное исследование](../../../docs/stride-mcp-integration-research.md).

Dependencies: [[feat-stride-source-foundation]]; принятый A1.1 в [[feat-stride-game-studio-authoring]]. Незаконченный A1.2 сохраняется в основной рабочей копии и не считается принятым.

Acceptance:

- Воспроизводимая сборка локального MCP сервера и адаптера для pinned Game Studio; реальные MCP initialize/tools/list/tools/call подтверждены клиентом.
- Явная адресация процесса, проекта, сессии, сцены и объекта; чтение состояния и свойств, изменение transform/custom property, Undo/Redo и сохранение через API редактора.
- Изменения видны в открытом Game Studio, переживают save/reopen и попадают в compiled asset. Окно не требует фокуса; две сцены не приводят к изменению первой попавшейся.
- Устаревшая revision, неверные идентификаторы, недоступный редактор и недопустимые свойства дают явный отказ без скрытой записи. Таймаут не оставляет неожиданную отложенную мутацию.
- Ошибки и настоящий viewport capture доступны либо имеют честную диагностированную границу. Клиентское подключение и команды воспроизведения документированы.
- Независимое read-only review, журнал реального API roundtrip и полная Release сборка редактора. Отдельно обозначены проверки MCP, GUI отображения, игры и оставшаяся A1 приёмка.

Origin: [[feat-stride-game-studio-authoring]]; [[research-open-engine-foundation]].
Follow-up: [[feat-stride-game-studio-authoring]].

## Resolution

Исправления двух P2 переданы на повторное review: `rat-engine` `122adde5`, Stride `88301e8` поверх upstream `e2c786a`. Добавлен native API состояния Save/Close/Dispose, адаптер проверяет его перед выполнением queued-команд. Реальные native session regressions, 73 MCP-вызова, транспорт и compiled parity прошли; fork создан, публикация патча и MCP ожидает независимого одобрения и parent gates.

Реализация `fcc73f6` передана на независимое read-only review. Пользователь дополнительно поручил сохранить MCP на GitHub и фиксировать там дальнейшие доработки редактора: MCP/адаптер/инструменты/документация — в `sancossi/rat-engine`; изменения исходников Stride при необходимости — отдельными проверенными коммитами в собственном fork, со ссылкой и pinned SHA в этом репозитории. Текущая публикация выполняется после review и итоговой проверки; upstream в этом срезе не изменялся.

Начато 2026-09-09 по явному «Давай делать MCP». Это согласованное изменение порядка: сначала MCP для редактора, затем возобновление A1.2 через проверенный API. Реализация в отдельной рабочей копии `C:/5_gamedev/rat-engine-mcp`, ветка `feat/stride-editor-mcp`; незакоммиченные карты в `C:/5_gamedev/rat-engine` сохранены. Один исполнитель, затем независимый reviewer; статусы ведёт parent.

## Bugs found

Повторное review подтвердило исправление обоих runtime P2. Остался узкий дефект переносимости `verify-session-state.ps1`: выбранный CheckoutPath не передавался как StrideBin при сборке qualification assembly; исправляется перед финальной приёмкой.

Независимое review `fcc73f6` выявило два P2: невызываемый SessionDisposed оставляет очередь активной после Session.Destroy; GUI Save не учитывается собственным activeOperation моста и может пересечься с MCP-мутацией. Исправления и регрессии выполняются в этом срезе до публикации. Предыдущие координатные попытки сдвига стены A1.2 не считаются пройденной приёмкой.
