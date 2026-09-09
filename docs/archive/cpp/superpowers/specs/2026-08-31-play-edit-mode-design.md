# Play/Edit mode toggle (S2)

**Status:** Approved in chat 2026-08-31 (pause EventRuntime in Edit).  
**Task:** [S2: Play/Edit mode toggle](https://app.notion.com/p/3cdf382736cc814cb8c0f4f0a5b7fac3)

## Decision

- `AppMode { Play, Edit }` in `rat_core` (`toggle_app_mode`, `app_mode_name`, `player_control_enabled`, `event_runtime_enabled`).
- Hotkey **F2** (edge); Tab reserved for ImGui docking.
- **Play:** WASD + interact + `EventRuntime::update` as today.
- **Edit:** no player control; **pause** `EventRuntime::update`; camera C + ImGui OK.
- Banner/dbgText + Inspector show `PLAY` / `EDIT`; mode survives resize.
- Out of scope: gizmos, hot-apply, selection.
