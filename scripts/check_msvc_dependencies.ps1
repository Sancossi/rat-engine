param([Parameter(Mandatory=$true)][string]$BuildDirectory)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent
$header = Get-Item -LiteralPath (Join-Path $repoRoot 'apps/editor/editor_document.hpp')
$originalTime = $header.LastWriteTimeUtc
$objectPaths = @(
    'apps/editor/CMakeFiles/rat_editor_logic.dir/editor_document.cpp.obj',
    'apps/editor/CMakeFiles/rat_editor_app.dir/editor_app.cpp.obj',
    'tests/CMakeFiles/rat_tests.dir/event_graph_edit_test.cpp.obj'
)
$before = @{}
foreach ($relative in $objectPaths) {
    $path = Join-Path $BuildDirectory $relative
    $before[$path] = (Get-Item -LiteralPath $path).LastWriteTimeUtc
}
try {
    # No content changes and no clean: only the dependency graph can trigger this rebuild.
    $header.LastWriteTimeUtc = [DateTime]::UtcNow
    & cmake --build $BuildDirectory
    if ($LASTEXITCODE -ne 0) { throw "Header dependency build failed: $LASTEXITCODE" }
    foreach ($path in $before.Keys) {
        if ((Get-Item -LiteralPath $path).LastWriteTimeUtc -le $before[$path]) {
            throw "Header changed but dependent object did not rebuild: $path"
        }
        Write-Output "Header dependency verified: $path"
    }
} finally {
    $header.LastWriteTimeUtc = $originalTime
}
