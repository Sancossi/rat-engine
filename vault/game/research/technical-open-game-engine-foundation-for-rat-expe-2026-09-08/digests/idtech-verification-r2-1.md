---
type: note
tags: [research, expedition, engine, digest, verification]
---

# Verification: id Tech 4

- **Verified, high:** GPLv3 permits charging for copies, but a conveyed modified/combined executable remains GPL and recipients must receive or be offered Corresponding Source without incompatible further restrictions.
  Sources: https://www.gnu.org/licenses/gpl-faq.en.html; https://www.gnu.org/licenses/gpl-3.0.en.html (FSF, 2007-06-29/n.d., accessed 2026-09-08).
- **Verified with correction, high:** Doom 3 BFG source omits game data, Steam integration, Bink playback and depth-fail stencil shadows. “Excluded code” also refers to third-party files with separate licenses; it does not mean all game code is absent.
  Source: https://github.com/id-Software/DOOM-3-BFG/blob/master/README.txt (id Software, 2012/n.d., accessed 2026-09-08).
- **Verified, fork-dependent, high:** dhewm3 builds x64 with CMake/VS2022 and supplied dependencies, though its official Windows download and integrated tools remain 32-bit. RBDOOM-3-BFG supplies a modern Win64 route and release package.
  Sources: https://github.com/dhewm/dhewm3; https://github.com/RobertBeckebans/RBDOOM-3-BFG (community projects, n.d., accessed 2026-09-08).
- **Verified, high:** maintained community runtime/tooling exists. dhewm3 1.5.5 shipped 2026-06-08; RBDOOM 1.6.0 shipped 2025-05-10; DarkRadiant 3.9.0 supplies x64 Windows builds. Tooling is fragmented and no maintained first-party id branch was found.
  Sources: https://github.com/dhewm/dhewm3/releases/tag/1.5.5; https://github.com/RobertBeckebans/RBDOOM-3-BFG/releases; https://www.darkradiant.net/ (accessed 2026-09-08).
- **Unverified, medium:** categorical unsuitability for greenfield development. Evidence supports a high-burden specialized option; no equal prototype proves absolute last place.

Strongest case: a PC-first, FPS-shaped project willing to publish the combined engine/game executable under GPL can reuse proven maps, entities, lighting, collision, scripting and modern community renderers instead of rebuilding low-level systems.
