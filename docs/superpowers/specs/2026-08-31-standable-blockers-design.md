# Standable Jumpable Blockers Design

## Goal

Allow the player to land on, stand on, walk off, and jump from the top of a
`jumpable` blocker while preserving existing raised `ground_y` terrain behavior.

## Design

Raised terrain remains owned by `SurfaceQuery`: small rises are walkable and
higher tiles are reachable by jumping. A jumpable blocker is a separate,
temporary support recorded in `JumpState` by blocker index. A descending swept
feet test may acquire that support only when crossing `top_y` from above while
the player's footprint overlaps the blocker.

While supported, the blocker top is the effective ground for vertical
integration. Horizontal movement remains valid because feet at `top_y` do not
collide with the blocker slab. Leaving its footprint releases support, preserves
world feet height, and starts coyote time. Jumping clears support and launches
from the blocker top. Hot apply, transfer, edit mode, and explicit jump resets
clear the support through the existing grounded-state reset.

## Acceptance

- Landing from above leaves the player grounded at `top_y`, without ejection.
- The player stays stable while idle and can move across the top.
- Walking off starts airborne fall with coyote time and no vertical teleport.
- Jumping from the top uses normal jump tuning.
- Approaching the blocker below `top_y` remains blocked.
- A jump that clears the blocker continues beyond it instead of landing.
- Raised terrain tiles remain jumpable and standable.
