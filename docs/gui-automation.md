# Editor GUI acceptance

`RAT_BUILD_GUI_TESTS=ON` builds and installs `rat-editor-gui-tests` beside the
editor. Both frontends link `rat_editor_app`, including its actual panels,
viewport handlers, ImGui and bgfx rendering. The runner requires explicit
`--user-data-dir` and `--artifact-dir`; `--data-root` defaults to executable-adjacent
data. Renderer selection is `--renderer software-d3d11` (Windows WARP) or
`--renderer software-opengl` (Linux Mesa with `LIBGL_ALWAYS_SOFTWARE=1`).

Windows software mode creates a D3D11 WARP device explicitly, verifies its actual
DXGI adapter has `DXGI_ADAPTER_FLAG_SOFTWARE`, and supplies that device to bgfx.
The report includes the actual adapter description. A software vendor request
alone is insufficient with the pinned bgfx DXGI adapter-zero fallback. Linux
currently reports software OpenGL as requested; local Linux verification remains
unavailable.

The infrastructure scenarios (`--scenario infrastructure`, `input-order`, and
`authoring-text`) exercise real mode buttons, quick keyboard/viewport taps,
Unicode fields, and multi-key text batches queued with Close/F5. They capture the
first Edit frame and the unsaved modal.

Additional implemented scenario arguments:

- `guard-{close,f5,reload,open}-{Save,Discard,Cancel}`: all twelve unsaved branches.
- `failed-save`: a named-target FileStore failure retains the modal and Close for retry.
- `map-backup`: the validated candidate survives the guard's Save rotating `.bak`.
- `slot-backup`: the real pause-menu restore applies the backup without rewriting main.
- `malformed-start`, `malformed-load`: clean initialization failure and transactional loading.
- `restart-create`, `restart-add`: run as two processes with the same user root, in that order.
- `creation-and-duplicate`: panel, viewport and context-menu creation plus rejected duplicate ID.
- `history-voxel`: no-op field/removal, undo-to-clean, aborted stroke preserving redo,
  real viewport voxel placement/removal and undo/redo.

- `graph`: actual pin gestures (self, source, empty release and two-click connection),
  add/connect/delete, one-gesture layout undo/redo, no-op redo preservation and persistence.
- `graph-active-close`: native Close settles a held node drag before checking dirty;
  Cancel followed by undo restores the original layout as one history entry.
- `ramp-{north,east,south,west}`: elevated solid side placement through the actual
  projection and picking pipeline; immutable camera poses expose the four faces.
- `bridge-{above,under,filled}`: real held movement onto queried thin support,
  through an underpass, and blocked by the corresponding filled-solid fixture.
- `play-modal`: previously advancing dirty Play pauses for the modal and resumes
  without replaying held F5 or accumulating paused simulation time.
- `unsupported-touch`, `unsupported-transfer`: Open unsupported drafts through UI,
  reject Apply/Play, repair through real widgets and successfully Apply/Play.
  EventTouch remains a visible disabled choice.

- `scale-{100,150,200}`: Unicode, viewport picking, pins, layout units and modals
  with immutable UI scale, scripted native window resize and a 1.25 framebuffer ratio.
  This exercises shared input/render dimensions; it does not change the OS DPI setting.
- `production-malformed-start` is orchestrated as the actual production executable,
  and must exit 1 cleanly. The C++ runner's negative initialization scenario is separate.

`scripts/gui_scenarios.json` maps all 40 scenarios to requirements. The installed
`run_gui_acceptance.py` runs fresh processes from an unrelated working directory,
including two restart phases sharing one project. It omits `--data-root` to exercise
executable-adjacent resources, verifies loaded audio and PNG dimensions, and checks
that package files and the unrelated working directory remain unchanged. It writes
a `gui-acceptance-report.json` beneath a fresh `gui-run-*` artifact directory and
returns nonzero on any failure. An explicit `--scenario` subset is marked incomplete.

After configuring with `-DRAT_BUILD_GUI_TESTS=ON`, run `ctest --test-dir build/dev-release
-L gui --output-on-failure`. The full test has label `gui` and timeout 300 seconds;
ordinary headless verification keeps GUI tests disabled. A GUI-only build still
copies/installs resources, but full acceptance requires the production editor too.

From an installed package and any working directory:

```text
python /absolute/package/run_gui_acceptance.py --user-data-dir /temporary/users --artifact-dir /temporary/artifacts --renderer software-d3d11
```

Linux CI uses `LIBGL_ALWAYS_SOFTWARE=1 xvfb-run -a python ... --renderer software-opengl`.
That Linux command is provided for CI and has not been verified locally on Windows.

`EditorFrameInput` carries held state and ordered key, character, mouse, wheel and
focus events. Native GLFW callbacks and scripted input use this same stream;
the GLFW ImGui backend does not install a competing callback stream or run its
polling NewFrame implementation. FrameCoordinator orders input, ImGui NewFrame,
simulation, audio, UI drawing and presentation. Viewport and gameplay consume
the same processed input state and cursor position as ImGui, including transitions trickled across
frames. Close/F5 pause immediately and remain latched until ImGui's pre-action
event queue is drained and the active widget has consumed its text. Trickling
remains enabled so multiple transitions cannot disappear.

Input positions and ImGui rectangles are logical pixels; camera picking uses
framebuffer pixels. Renderer view rectangles and scissors use framebuffer pixels.
UI style and font scale start from fresh base state for each immutable launch.

The observer records submitted ImGui item rectangles, labels, windows and status
through the pinned ImGui instrumentation hooks. It cannot activate widgets.
Read-only app observations expose authored content, history availability, modal
and error state, simulation state, graph gesture state and real world projection.
Fixtures may configure initial files, player and layout before launch. Subsequent
workflow actions must use input and visible widgets, never document commands.

Screenshots use bgfx's asynchronous callback, with BGRA format, source pitch and
origin passed to the existing bimg PNG writer. The callback remains owned by the
renderer through shutdown. Missing selectors, initialization failure or capture
timeout fail with a nonzero exit code; there are no skipped GUI tests. Artifacts
include PNGs, `editor.log`, read-only JSON snapshots and `scenario-report.json`.
