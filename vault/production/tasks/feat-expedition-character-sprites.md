---
type: task
area: Game
status: Not started
task_type: Feature
sprint:
due:
tags: [task, expedition, art, sprites]
---

# Три отдельных облика и направленная ходьба

Intent: Подготовить по пользовательскому ориентиру три различимых облика партии и корректное воспроизведение ходьбы через native asset workflow, начиная с одного проверенного героя.

Specification: [План спрайтов](../../../docs/stride-character-sprite-plan.md); [[rat-expedition-character-reference]]; [[rat-expedition-art-audio]]; [[GDD]].

Dependencies: [[feat-stride-game-studio-authoring]]; [[design-expedition-character-sprites]]. Ограниченная квалификация после A1 до массового арта E3, вне спринта. Не новая зависимость A1/P2; полный E3 сохраняет P7 и A1.

Acceptance:

- Пилот фиксирует reference/direct-import решение, происхождение, native pixel grid/common canvas/foot pivot, направления и соответствие образа каноническому герою без назначения по одежде.
- Проверены alpha и светлая шерсть; сохранены оригинал, редактируемый source и экспорт. Три наблюдаемые позы не объявляются готовым loop: порядок/cadence и отличие от A7 walk 4 / idle 2 явно приняты по preview.
- Native import/edit/preview/undo/redo/save/reopen/reimport работает через A1; игра и ZIP используют тот же авторский источник, устойчивые skin/clip ids и три отдельных набора вместо tint-копий.
- Ходьба следует фактическому движению и существующей FSM, останавливается у стены/в паузе, сохраняет последнее направление. Спутники используют исторические направление/стойку/фазу собственного пути, portal/recovery корректно сбрасывают представление.
- Каждый силуэт проверен при прежних body dimensions/camera, у стен, в лазе, на лестнице, на/под мостом. Пробелы crouch/climb/fall закрыты принятыми dedicated кадрами либо явно проверенными fallback; атака/бой не реализуются этой карточкой.
- Реальные editor/runtime/ZIP проверки 720p/1080p, короткое движение, invalid/missing asset сценарии, независимое ревью и Release редактор отражены в evidence. Ручная визуальная оценка, время подготовки и credits записаны до массового E3.

Origin: [[design-expedition-character-sprites]]; [[rat-expedition-character-reference]]; [[feat-stride-game-studio-authoring]].
Follow-up: [[expedition-vertical-slice]].

## Resolution

Реализация не начиналась. Первый результат — один принятый пилот; после него остальные два облика по тому же шаблону. Статусы и назначение в спринт меняет ведущий.

## Bugs found

Не проверено: карточка планирует будущую работу, текущий runtime не менялся.
