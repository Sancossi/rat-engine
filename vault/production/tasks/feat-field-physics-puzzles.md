---
type: task
area: Game
status: Not started
task_type: Feature
sprint:
due:
tags: [task]
---

# feat: Field physics puzzles

Intent: небольшие «физические» загадки на карте: двигать ящик (в том числе героем), нажимать напольные панели героем и тяжёлыми предметами, проваливающиеся полы, забытые двери, которые выбиваются только с разбегу. Не MVP-слайс и не текущий спринт — задаёт форму коллизий.

Acceptance (позже): ящик — динамическое тело, толкается игроком и упирается в стены/заборы/кубы; панель на полу срабатывает от массы/стоящего тела; плита может обрушиться под весом; дверь-solid ломается при ударе со скоростью выше порога. Контекст: [[GDD]] field action, [[edge-walls-passable-from-adjacent-side]].

Origin: chat (collider design). Collision slice: [[edge-walls-passable-from-adjacent-side]].
