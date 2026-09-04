# Play walks event graph (2026-09-04)

Sprint 12 design. Supersedes [[research: Event node graph vs bytecode]] Play-truth: graph is authoring-only.

## Decision

- **Play truth** = `EventPage.graph` (nodes + edges). `EventRuntime` walks nodes, not `commands[]`.
- **Legacy JSON** with only `commands`: `commands_to_graph` in the loader, then runtime never reads the list.
- After drop-commands: save omits `commands`. Apply no longer compiles graph → list.
- Trigger + page conditions stay outside the graph.
- No Ruby/JS VM. No imgui-node-editor.
- Parallel budget: **nodes per frame** (`kMaxParallelCommandsPerFrame` keep the number 32, rename in code to nodes).

## Reverse-compile (`commands_to_graph`)

Input: `std::vector<Command>`. Output: `EventGraph`.

- Linear ops → one node each, sequence edges `entry → n1 → n2 → … → exit`.
- `conditional_branch` → node `conditional_branch`; recurse `then_commands` / `else_commands`; then-edge to first then node (or join if empty); else-edge likewise; both tails sequence to a **join** (the next command after the branch, or `exit`).
- Node ids: `n1`, `n2`, … unique. Map `Command` fields onto `EventGraphNode` the same way `command_from_node` does in reverse.
- Golden: `compile_event_graph(commands_to_graph(cmds)).commands` equals `cmds` for linear + one nested branch + set_move_route + wait.

Loader: if page has `commands` and no `graph` (or empty graph), set `page.graph = commands_to_graph(page.commands)`.

## Runtime walker

Replace `StackFrame { commands*, index }` with:

```
struct StackFrame {
  std::string node_id; // current node, or "entry" before first step
};
```

`start_page`: require `page.graph`; current = sequence successor of `entry`.

Each budget tick (same yield rules as today):

- `exit` or missing successor → pop / finish interpreter.
- Kind dispatch = today’s `exec_command` / `exec_set_move_route`, but payload from `EventGraphNode` (or `command_from_node` helper reused).
- ShowText / Wait / Move Route still yield (`return false` / wait_frames / route lerp).
- `conditional_branch`: evaluate condition; follow then-edge else else-edge (else may be missing → treat as join/exit like empty else_commands).
- Comment: no-op, continue.

Debug: `command_index` may become `node_id` string or leave unused; do not break tests that only check wait_frames / overlay.

## Drop commands (after walker green)

- `EventPage` optional/empty `commands`; dump_page omits `commands` or writes `[]`.
- Schema: `graph` required for Play pages; `commands` deprecated/ignored.
- Remove `compile_event_graphs_for_apply` from save/apply (keep `compile_event_graph` only if tests still use it as reverse-compile oracle — or switch goldens to walker).
- Migrate `data/maps/*.json` to graph-only when convenient in this chore.

## In-node widgets (after walker)

ImGui controls **on the node body** (not only the panel below). Variable height. Pins stay. Pan/zoom unchanged. Hide command-list fallback when graph exists.

## Out of sprint

Choice menus, portraits, fade, animation nodes from the reference screenshot. Label+Jump. Event touch.
