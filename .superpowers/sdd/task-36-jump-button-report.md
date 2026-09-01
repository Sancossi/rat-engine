# Task 36 report: Jump button does not always fire

## What you implemented

- Root cause: editor samples `jump_pressed` as a display-frame edge, then `drain_simulation_catch_up` only feeds that edge into `tick()`. When display FPS > sim rate (or leftover accumulator is short), that frame runs **zero** ticks. `previous_buttons_` already consumed the edge, so the next drain has `jump_pressed == false` and the launch never happens.
- `jump_press_pending_` already held a press across ticks, but it was only set inside `tick()`, so a 0-tick drain never latched it.
- Fix: `SimulationSession::note_jump_pressed()` plus `drain_simulation_catch_up` calls it when the display frame has `jump_pressed`, including `to_run == 0`. Later ticks still see a held key without a repeated edge.
- Did **not** change `WantCaptureKeyboard` / `clear_pending_input` (that path is still “capture drops buffer”). Walking still works while captured-gated input would also kill WASD, so capture was not the walking miss.

Vault card status was **not** changed. No push.

## What you tested and results

| Command | Result |
| --- | --- |
| `rat_tests.exe "*zero-tick*"` before latch | RED: `CHECK_FALSE(session.jump().grounded)` failed (`grounded` stayed true) |
| `rat_tests.exe "*zero-tick*"` after latch | GREEN, 1 case / 10 assertions |
| `rat_tests.exe "[sim]"` | GREEN, 10 cases / 122 assertions |
| `ctest --test-dir build --output-on-failure` | `100% tests passed, 0 tests failed out of 327` |

## TDD Evidence

**RED:** `drain_simulation_catch_up latches jump edge across a zero-tick display frame` — press on a half-dt display frame (0 ticks), then held (no edge) on the next half-dt frame (1 tick); player stayed grounded.

**GREEN:** same case launches; existing hitch “first tick only” jump test still matches 1 press + 2 held ticks.

## Files changed

- `src/engine/include/rat/simulation_session.hpp`
- `src/engine/src/simulation_session.cpp`
- `tests/simulation_session_test.cpp`

Not staged: vault, other `.superpowers/sdd/*`, `build/`.

## Self-review findings

- **Completeness:** Headless repro of the editor display-loop miss; drain is the shared editor path so `EditorApp` needs no extra sampling patch.
- **Quality:** Does not clear jump buffer when `jump_pressed` is false; hitch retrigger still prevented by zeroing the edge after the first tick that runs.
- **YAGNI:** No ImGui `WantTextInput` split, no interact 0-tick latch (same class of bug, out of scope).

## Concerns

1. **Interact** has the same 0-tick drop (`BufferedPress` is only pushed inside `tick`). Not fixed here.
2. **WantCaptureKeyboard** still zeros pending + buffer. If a panel is focused, Space still does nothing by design; playtest “ImGui focused” may still feel broken until a follow-up (e.g. gate on `WantTextInput` only).
3. Polling `glfwGetKey` can still miss a tap that is down-and-up between display frames; not reproduced headless.
