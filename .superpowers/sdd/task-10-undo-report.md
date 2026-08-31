# Task 10 report: feat: Edit undo/redo command stack

## What you implemented

GPP Command for **Edit**, not input. Height-grid stays out of the stack.

- `EditCommand` / `EditHistory` in `rat_core`: `execute` applies and pushes undo (clears redo), `undo`/`redo` return false if empty, `clear()`, `can_undo`/`can_redo`. No singleton; owner is `EditorApp`.
- Concrete commands: place/delete/move for `BlockerDef` and `EventDef`. Move uses `translate_aabb_on_grid` / `translate_event_on_grid`. Delete revert inserts at the stored index.
- Editor place / grid-move / delete of blockers and events call `execute` instead of raw `push_back`/`erase`/translate. Grow/shrink/snap, jumpable, and page/text tweaks still mutate in place (YAGNI).
- `clear()` after **successful** `hot_apply_map_path` (covers init load). Play does not record: Edit UI only, and Ctrl+Z/Y apply only when `app_mode_ == AppMode::Edit`.
- Ctrl+Z undo, Ctrl+Y or Ctrl+Shift+Z redo via `InputButtons`/`InputFrame` edges, gated by `!WantCaptureKeyboard`. After undo/redo: `set_blockers` / `set_events` + greybox sync.

## What you tested and results

- Focused: `.\build\tests\rat_tests.exe "[edit]"` → **76 assertions in 7 test cases, all passed**
- Input: `.\build\tests\rat_tests.exe "[input]"` → **62 assertions in 12 test cases, all passed**
- `[blocker_edit]` / `[event_edit]` stayed green
- Full (before commit): `.\build\tests\rat_tests.exe` → **1722 assertions in 206 test cases, all passed**
- `rat-editor` built after editor wiring

Cases (`tests/edit_history_test.cpp`, tags `[unit][edit]`, tiny `MapData`, no GLFW):

1. Place blocker → undo removes it → redo restores AABB
2. Move blocker by 1 tile → undo restores AABB → redo moves again
3. Delete event → undo restores at the same index (id/tile) → redo deletes
4. `clear()` after execute → `undo` is false; map left as after last execute
5. New `execute` after undo clears redo (placed AABB is the new one, not the undone one)
6. Place event + delete blocker round-trip ids/AABB
7. Move event by one tile undo/redo restores tile

Plus `InputFrame` undo/redo edges, held = no press, `keyboard_captured` suppresses both.

## TDD Evidence

**RED** (API missing; tests did not compile):

```
cmake --build build --target rat_tests
edit_history_test.cpp(1): fatal error C1083:
  rat/edit_history.hpp: No such file or directory
```

**RED** (input fields missing):

```
input_test.cpp(106): error C2039: "undo": not a member of "rat::InputButtons"
```

**GREEN** (focused, after history + factories):

```
.\build\tests\rat_tests.exe "[edit]"
All tests passed (76 assertions in 7 test cases)
```

**GREEN** (full suite + editor before commit):

```
cmake --build build --target rat_tests rat-editor
.\build\tests\rat_tests.exe
All tests passed (1722 assertions in 206 test cases)
```

## Files changed

- `src/engine/include/rat/edit_history.hpp` (new)
- `src/engine/src/edit_history.cpp` (new)
- `src/engine/CMakeLists.txt`
- `src/engine/include/rat/input.hpp` — `undo`/`redo`, `undo_pressed`/`redo_pressed`
- `src/engine/src/input.cpp` — edges gated by `!keyboard_captured`
- `apps/editor/editor_app.hpp` — owns `EditHistory`
- `apps/editor/editor_app.cpp` — execute on place/move/delete, Ctrl+Z/Y, `clear()` on hot-apply
- `tests/edit_history_test.cpp` (new)
- `tests/input_test.cpp`
- `tests/CMakeLists.txt`
- `vault/production/tasks/feat-edit-undo-redo.md` — `Done`, Resolution, Bugs found: none
- `vault/engine/Game Programming Patterns.md` — Command (редактор) row
- `vault/engine/Systems Index.md` — Edit history row

## Self-review

- Commands restore vector indices (delete insert-at-index); editor clamps selection after undo/redo
- Play cannot push: Edit-only ImGui buttons; undo/redo keys ignored unless `AppMode::Edit`
- Failed hot-apply does not `clear()`
- Grow/shrink/jumpable/page fields are not commands (per brief YAGNI)
- No GLFW in `rat_core`; tests use `MapData` only

## Concerns

Grow/shrink/snap and jumpable/text edits are still irreversible. Intentional YAGNI; viewport mouse edit is a separate card. No follow-up filed.

## Commit

`0dd7cf1` Add an Edit command stack so blocker and event edits can be undone.

## Review fix: skip EventRuntime reset on blocker edits

`apply_edited_map` always called `set_events`, which clears interpreters / `active_message_` / `autorun_lock_`. Blocker place/move/delete (and their undo/redo) now skip `set_events`. Event commands still call it. Grow/shrink was already `set_blockers` only.

- `EditCommand::mutates_blockers()` / `mutates_events()`
- `execute` / `undo` / `redo` return `EditApplyResult` (`applied` + flags)
- Editor applies only the matching setter + greybox sync

### TDD Evidence

**RED** (`mutates_blockers` / `EditApplyResult` missing):

```
edit_history_test.cpp(159): error C2039: "mutates_blockers": is not a member of "rat::EditCommand"
edit_history_test.cpp(187): error C2039: "EditApplyResult": is not a member of "rat"
```

**GREEN** (focused `[edit]`):

```
.\build\tests\rat_tests.exe "[edit]"
All tests passed (106 assertions in 10 test cases)
```

**GREEN** (contract: `set_blockers` keeps message/lock, `set_events` clears):

```
.\build\tests\rat_tests.exe "*set_blockers keeps*"
All tests passed (13 assertions in 1 test case)
```

**GREEN** (full suite + editor before commit):

```
cmake --build build --target rat_tests rat-editor
.\build\tests\rat_tests.exe
All tests passed (1790 assertions in 217 test cases)
```

New `[edit]` cases: command flags (blocker vs event), execute/undo/redo of a blocker command report blockers-only, same for events. `[events]` documents the `set_blockers` / `set_events` contract `apply_edited_map` relies on.

Vault card stays `In progress`. Grow/shrink undo still out of scope.

