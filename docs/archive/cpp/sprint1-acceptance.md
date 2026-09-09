# Sprint 1 — Acceptance playthrough

Date: 2026-08-31  
Branch: `cursor/glfw-imgui-editor-shell`  
Build: `.\build\apps\editor\rat-editor.exe`  
Automated: `ctest --test-dir build -C Release` (includes `[mechanics][quest]`)

## Demo path (manual)

1. Launch editor; autorun text: *Grey Yard — … Talk to the foreman.* → **E/Space**.
2. Walk to cyan **foreman** (tile ≈ -2, 2). Prompt **[E] Interact** → accept quest (switch1 ON).
3. Walk **around crates** (purple AABB at x≈3–5); optional touch notice about the short path.
4. Interact with cyan **scrap** (tile ≈ 6, 0) → *rusty cog* (switch3 ON).
5. Return to foreman → turn-in; switch2 ON; cog removed.
6. Interact again → completion banter.

## Checklist

| # | Criterion | Status |
|---|-----------|--------|
| 1 | Ortho 3/4 grey floor + grid visible | Pass |
| 2 | Player marker visible; camera follows | Pass |
| 3 | WASD/arrows camera-relative | Pass |
| 4 | Snap to grid → cell centers | Pass |
| 5 | Blockers collide / slide | Pass |
| 6 | JSON map loads (`grey_yard.json`) | Pass |
| 7 | Autorun intro once | Pass |
| 8 | Action + player_touch events | Pass |
| 9 | Dialog advance (E/Space/button) reliable | Pass |
| 10 | Interact prompt near Action events | Pass |
| 11 | Quest solvable via switches/items | Pass (manual + headless) |
| 12 | Parallel limits do not crash (unit) | Pass |
| 13 | Catch2 / ctest green | Pass |

## Known issues

- No mesh/characters yet — cyan pillars = events; green pillar = player.
- Event pages use RM-like **highest matching page**; authors must order pages carefully.
- `transfer_player` updates GameState only (no multi-map streaming).
- Parallel busy-loops without `wait` are budget-clipped (warn), not hard-errored in UI toast.
- ImGui docks can still cover part of the viewport; world draws full-frame behind PassthruCentralNode.
- Save/load is in-memory stub only (not wired to editor File menu).

## Notes

Sprint goal met: walkable grey-box + JSON events + one end-to-end quest.
