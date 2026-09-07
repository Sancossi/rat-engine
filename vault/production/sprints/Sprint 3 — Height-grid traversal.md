---
type: sprint
status: Done
dates: 2026-09-01/2026-09-14
goal: Height-aware maps, smooth stairs, and responsive traversal jump
current: false
tags: [sprint]
notion_id: 3cdf3827-36cc-8144-a2ad-feaa7fd204a9
---

# Sprint 3 — Height-grid traversal

Цель: добавить управляемую вертикальность без полноценной rigid-body физики.

DoD:

- map JSON хранит высоты клеток и лестничные зоны;
- игрок плавно поднимается по лестницам;
- отзывчивый прыжок преодолевает низкие препятствия;
- коллизии, события, markers, save/load и hot-apply учитывают высоту;
- acceptance-сцена проходится без рестарта.

Вне scope: несколько поверхностей в одной XZ-точке, мосты и многоэтажные интерьеры. API ground query проектируется с учётом будущего stacked-surface расширения.

## Итог

Sprint закрыт 2026-08-31. Реализованы height-grid в map JSON, ground query, smooth stairs/ramps, height-aware коллизии и события, elevation в Edit, отзывчивый прыжок (coyote/buffer). Acceptance пройден: лестницы, прыжок и elevated event без рестарта. Follow-up баги [[landing-on-jumpable-blocker-slides-player-off]] и [[ramp-allows-entry-from-side]] — Fixed. Миграция хаба в vault: [[feat-migrate-notion-hub-to-obsidian-vault|feat: migrate Notion hub to Obsidian vault]]. Хвосты height-grid polish в Sprint 4 не переносились.
