# Authoring transactions

`EditApplyResult::ok` reports successful execution; `changed` reports a content
change. Successful no-ops carry no mutation flags and neither append history nor
clear redo. Commands apply to a candidate map; a failed redo retains both the map
and its redo entry. Aborted and net-zero strokes preserve redo.

`authoring_snapshot` compares authored fields without validation or graph
compilation. Graph-backed pages exclude derived command caches; command-only
legacy pages include commands. Array order and optional field presence are
preserved, signed zero is normalized. Node `layout: {x, y}` is optional,
finite authoring metadata, separate from transfer-player coordinates. Legacy
maps remain readable. Runtime map fingerprints must exclude this layout.

`EditorDocument` retains a clean content checkpoint across ordinary edits and
undo/redo. Preview is visual until its normal replace command commits. Switching
selection, executing another command, saving, or requesting a guarded action
settles the preview first; invalid previews retain their data and diagnostic.
Graph compilation returns a separate result and does not replace the document.
Rendering authoring changes does not reload the simulation VM; Apply and Enter
Play validate and apply explicitly. A structurally valid but unsupported or
unfinished authored map opens in Edit for repair; malformed input is rejected
before replacing the document.

`EditorActionController` queues Close, LoadMap and RestoreMapBackup. EditorApp
pumps it after UI field processing, settles the active stroke/preview, and uses
Save / Discard / Cancel when dirty. Save failures keep the pending action. Native
close requests are consumed and reset; the application exits after the controller
performs Close. Pending actions pause simulation and disable editing. Dismissal
clears pending input, edge state and the fixed-step accumulator.

Map backup content is read and structurally validated before prompting, because
Save rotates `.bak`. Restore retains the main path and compares against the main
file checkpoint; it does not overwrite the main file until Save. Save-slot backup
loading validates a temporary GameState and applies through SimulationSession;
neither load nor restore rewrites the slot. There is no automatic fallback.

Headless regressions cover these document/history/controller contracts, malformed
loads, save failure, backup rotation, restored dirty state and identifier reuse
after reload. `EditorApp(FileStore&)` supplies the same file operations to real UI
handlers and supports failure injection for the GUI runner. Actual native focus,
modal, gesture and rendering acceptance belongs to the Sprint 15 GUI suite.
