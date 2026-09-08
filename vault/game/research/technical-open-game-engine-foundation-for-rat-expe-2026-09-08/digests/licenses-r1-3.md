---
type: note
tags: [research, expedition, engine, digest]
---

# Digest: лицензии, версии и жизнеспособность

- claim: Godot stable 4.7.2 опубликован 2026-08-18, а 4.8-dev4 — 2026-08-26, что показывает активный stable/development cadence.
  source: https://godotengine.org/download/archive/
  publisher: Godot Engine
  pub_date: 2026-08-26
  accessed: 2026-09-08
  confidence: high
  class: version
- claim: Godot публикует полный исходный код под MIT, поддерживает Windows и единый 2D/3D workflow; proprietary commercial games разрешены при сохранении copyright/license notice.
  source: https://github.com/godotengine/godot
  publisher: Godot Engine contributors
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: license
- claim: Stride newest observed release 4.4.0-beta6 является prerelease от 2026-09-08; наблюдаемая стабильная линия — 4.3.0.2507.
  source: https://github.com/stride3d/stride/releases
  publisher: Stride contributors
  pub_date: 2026-09-08
  accessed: 2026-09-08
  confidence: high
  class: version
- claim: Stride — публичный C# engine/editor под MIT; proprietary commercial distribution разрешён при сохранении уведомлений и проверке third-party notices.
  source: https://github.com/stride3d/stride
  publisher: Stride contributors
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: license
- claim: Wicked Engine — публичный C++ engine/editor под MIT; observed release v0.72.113 от 2026-08-24 показывает активное сопровождение, но с концентрацией вокруг ведущего maintainer.
  source: https://github.com/turanszkij/WickedEngine
  publisher: Wicked Engine contributors
  pub_date: 2026-08-24
  accessed: 2026-09-08
  confidence: high
  class: ecosystem
- claim: Windows-слой Wicked открыт, но заявленные console extensions остаются private; это не блокирует текущую Windows-цель.
  source: https://github.com/turanszkij/WickedEngine
  publisher: Wicked Engine contributors
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: compatibility
- claim: O3DE release 26.05.0 опубликован 2026-05-27 с Windows/Linux/source downloads и сотнями исправлений; часть новых систем остаётся experimental.
  source: https://www.docs.o3de.org/docs/release-notes/
  publisher: Open 3D Engine contributors
  pub_date: 2026-05-27
  accessed: 2026-09-08
  confidence: high
  class: version
- claim: O3DE — крупный публичный C++ codebase под Apache-2.0; closed-source games возможны при соблюдении license/notice/modification требований и аудите bundled components.
  source: https://github.com/o3de/o3de
  publisher: Open 3D Engine contributors
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: license
- claim: Масштаб и активность O3DE подтверждают жизнеспособность, но официальные материалы не доказывают преимущество производительности разработки для малой solo RPG.
  source: https://github.com/o3de/o3de
  publisher: Open 3D Engine contributors
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: medium
  class: ecosystem
- claim: Официальный Doom 3 BFG source — исторический GPL-3.0 drop без game data, releases и современного first-party toolchain; он направляет к community forks.
  source: https://github.com/id-Software/DOOM-3-BFG
  publisher: id Software
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: ecosystem
- claim: GPL-3.0 допускает коммерческую продажу, но производный распространяемый executable обычно требует corresponding GPL source и исключает несовместимые дополнительные ограничения.
  source: https://github.com/id-Software/DOOM-3-BFG
  publisher: id Software
  pub_date: n.d.
  accessed: 2026-09-08
  confidence: high
  class: license

Contradictions: frequent releases не равны production stability; MIT/Apache означают compliance, а не отсутствие условий; GPL разрешает продажи, но плохо сочетается с закрытым монолитным executable.

Leads: одинаковый spike Godot 4.7.2 и Stride 4.3; audit exact third-party notices; Wicked оценивать по editor/import workflow; id Tech route — только через конкретный maintained fork и legal review.

Gaps: не установлены LTS и succession guarantees Stride/Wicked, first-party maintained id Tech 4 branch и измеренная цена миграции.
