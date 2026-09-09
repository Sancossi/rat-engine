# Editor launch and package contract

Stage 6 foundation, 2026-09-07. This delivers build/path/package infrastructure;
real GUI acceptance and CI package execution are subsequent stage 6 work.

`parse_editor_launch_arguments` and `resolve_editor_launch_options` belong to
headless `rat_editor_logic`. The parser accepts separate-value `--data-root`,
`--user-data-dir`, `--map`, plus `--help`. Unknown/duplicate/missing options return
an error. The resolver takes an injected launch cwd, executable path, optional log
override and platform default user root; it performs no I/O or environment reads.
An explicit user root works without a platform default. Resolution is lexical,
absolute and performed once; user paths need not exist before startup.

`platform/launch_environment.cpp` captures the process values. Windows arguments
come from the wide command line, executable from GetModuleFileNameW, user default
from LocalAppData. Linux uses /proc/self/exe and absolute XDG_DATA_HOME or HOME.
Help/argument errors return before environment discovery. An explicit user-data
argument bypasses default-user-directory discovery on both platforms.

`EditorApp::init(const EditorLaunchOptions&)` takes owned resolved UTF-8 paths:

| Purpose | Default |
| --- | --- |
| Bundled resources | executable directory / data |
| Initial map | data root / maps/grey_yard.json |
| User root | LocalAppData/rat-engine or XDG_DATA_HOME/rat-engine |
| Save slot and its backup | user root / saves/slot1.ratsave[.bak] |
| Log | user root / logs/rat.log; explicit RAT_LOG_PATH overrides |
| Debug snapshot | user root / debug/rat-debug.json |
| ImGui settings | user root / imgui.ini |

The app checks absolute paths, creates output parent directories, and stores the
strings for the lifetime of ImGui and the logger. Font/audio/map reads use data
root. FileLogSink converts UTF-8 paths through filesystem::path; FileStore already
does so. The pinned ImGui file adapter uses the wide Windows file API internally.
Our MSVC targets use `/utf-8`; dependency build flags are left unchanged.

The main executable only resolves process options and starts `rat_editor_app`.
The shared library contains the same real app, panels, native window, ImGui/bgfx
backend and audio sink. Its injectable FileStore constructor remains available.
The next GUI slice can resolve an isolated fixture/data/profile, pass these options,
and add scripted frame input without substituting document commands for gestures.
No software-renderer/input-script implementation is included in this foundation.

`RAT_BUILD_RENDERER` controls renderer/bgfx independently. Editor or GUI enables
GLFW/ImGui/audio and requires renderer. All off leaves core and editor logic with
only JSON and optional Catch2. The fresh headless configure check asserts both the
target graph and the absence of fetched graphics/audio directories.

Install and CPack place the executable and data together, include licence notices,
and bundle required MSVC runtime libraries. Resource paths contain no source-tree
definition. A clean downstream CI job without checkout will validate the downloaded
package from an unrelated cwd through the real GUI runner. Until that runner exists,
package creation and `--help` checks are not reported as interactive acceptance.

## Foundation verification

Windows Release and fresh headless Release each passed 703/703 tests, including
path/default/relative/Unicode parsing and Unicode log writing. The MSVC header
rebuild check verified editor logic, the shared app object and a dependent test.
The renderer-only configuration built rat_engine with only bgfx/JSON source
inputs and no GLFW/ImGui/audio targets. Install and ZIP creation succeeded;
installed `--help --user-data-dir ./profile` ran from an unrelated temporary cwd.
This checks the process entrypoint, not map/font/render initialization. Full package
GUI acceptance and Linux execution remain for the next slices. Shutdown now guards
an uninitialized ImGui platform backend when startup fails before UI creation.
