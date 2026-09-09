param(
    [ValidateSet('dev-release', 'dev-debug', 'headless-release', 'gui-release', 'game-release')][string]$Preset = 'dev-release',
    [string]$Python,
    [switch]$ConfigureOnly,
    [switch]$CheckHeaderDependencies,
    [switch]$Benchmarks
)
$ErrorActionPreference = 'Stop'
$repoRoot = Split-Path $PSScriptRoot -Parent

function Invoke-Checked {
    param([string]$Executable, [string[]]$Arguments)
    & $Executable @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Executable failed with exit code $LASTEXITCODE" }
}

if (-not $Python) {
    $pythonCandidates = foreach ($name in @('python3', 'python', 'py')) {
        Get-Command $name -All -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Source
    }
    foreach ($candidate in $pythonCandidates) {
        # WindowsApps aliases are launchers, not usable installed interpreters.
        if ($candidate -like '*\Microsoft\WindowsApps\*') { continue }
        try {
            & $candidate -c 'import sys; sys.exit(0 if sys.version_info >= (3, 9) else 1)' 2>$null
            if ($LASTEXITCODE -eq 0) { $Python = $candidate; break }
        } catch { continue }
    }
    if (-not $Python) { throw 'Python 3.9+ was not found. Install Python or pass -Python with its executable path.' }
}

# Invoke the structured VS environment exporter. This avoids shell interpolation
# of installation paths and captures only env variables, not shell startup output.
$vswherePath = Join-Path ${env:ProgramFiles(x86)} 'Microsoft Visual Studio/Installer/vswhere.exe'
if (-not (Test-Path -LiteralPath $vswherePath)) { throw 'Install Visual Studio C++ Build Tools (vswhere is missing).' }
$vsInstall = & $vswherePath -latest -products '*' -requires Microsoft.VisualStudio.Component.VC.Tools.x86.x64 -property installationPath
if ($LASTEXITCODE -ne 0 -or -not $vsInstall) { throw 'No Visual Studio installation with the C++ workload was found.' }
$devShell = Join-Path $vsInstall 'Common7/Tools/Microsoft.VisualStudio.DevShell.dll'
Import-Module $devShell
Enter-VsDevShell -VsInstallPath $vsInstall -SkipAutomaticLocation -DevCmdArguments '-arch=x64 -host_arch=x64'
$env:VSLANG = '1033' # Prefer English when installed; CMake also probes the actual localized prefix.

Push-Location $repoRoot
try {
    Invoke-Checked $Python @('scripts/check_vault.py')
    Invoke-Checked $Python @('-m', 'unittest', 'discover', '-s', 'scripts/tests')
    $configureArgs = @('--preset', $Preset)
    if ($Benchmarks) { $configureArgs += '-DRAT_BUILD_BENCHMARKS=ON' }
    Invoke-Checked 'cmake' $configureArgs
    if (-not $ConfigureOnly) {
        Invoke-Checked 'cmake' @('--build', '--preset', $Preset)
        if ($CheckHeaderDependencies -and $Preset -ne 'headless-release') {
            & (Join-Path $PSScriptRoot 'check_msvc_dependencies.ps1') -BuildDirectory (Join-Path $repoRoot "build/$Preset")
        }
        Invoke-Checked 'ctest' @('--preset', $Preset)
    }
    Write-Output "Build directory: $(Join-Path $repoRoot "build/$Preset")"
    if (-not $ConfigureOnly -and $Preset -eq 'game-release') { Write-Output "Game: $(Join-Path $repoRoot 'build/game-release/apps/game/rat-game.exe')" }
    if (-not $ConfigureOnly -and $Preset -notin @('headless-release', 'game-release')) { Write-Output "Editor: $(Join-Path $repoRoot "build/$Preset/apps/editor/rat-editor.exe")" }
} finally {
    Pop-Location
}

# Native stderr (e.g. unittest progress) is not a failed verification.
exit 0
