---
type: note
tags: [research, expedition, engine, digest]
---

# Digest: текущая контрольная база rat-engine

- claim: rat-engine распространяется по разрешительной MIT License, поэтому текущая игра может оставаться закрытой или открытой при сохранении уведомления.
  source: ../../../../../LICENSE
  publisher: Sancossi
  pub_date: 2026
  accessed: 2026-09-08
  confidence: high
  class: license
- claim: Незавершённый P1.1 уже создаёт отдельный rat-game, game-release preset, дисковый PNG-спрайт и Windows ZIP; проверка зарегистрировала 721 CTest, из которых 720 прошли и один symlink-тест пропущен из-за локальной привилегии Windows.
  source: ../../../../../docs/audits/2026-09-08-rat-expedition-traversal.md
  publisher: Rat Expedition project
  pub_date: 2026-09-08
  accessed: 2026-09-08
  confidence: high
  class: delivery
- claim: Текущий стек уже имеет fixed-tick simulation, height-aware surfaces, ramps, ladders, JSON maps, CMake/CTest и собственный редактор; оставшаяся цена P1 сосредоточена на posture policy, смене сцен, спутниках и перекрытиях.
  source: ../../../../../docs/rat-expedition-architecture.md
  publisher: Rat Expedition project
  pub_date: 2026-09-08
  accessed: 2026-09-08
  confidence: high
  class: migration
- claim: Переход на любой кандидат потребует либо преобразования текущих карт/данных и повторной реализации тестируемых игровых правил, либо отказа от уже работающего P1.1; автоматическая переносимость C++ runtime не установлена.
  source: ../../../../../docs/rat-expedition-engine-comparison.md
  publisher: Rat Expedition project
  pub_date: 2026-09-08
  accessed: 2026-09-08
  confidence: high
  class: migration

Leads: сравнивать кандидатов с фактическим game-release и 720 passing tests; отдельно оценить стоимость доказательства спрайтовых перекрытий и headless gameplay regressions.

Gaps: календарная стоимость завершения P1 на rat-engine и миграции не измерена одинаковыми spikes.
