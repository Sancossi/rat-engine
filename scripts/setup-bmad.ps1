param()
$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
$toolRoot = Join-Path $projectRoot 'tools/bmad'
$versions = Get-Content -LiteralPath (Join-Path $toolRoot 'versions.json') -Raw | ConvertFrom-Json
Push-Location $projectRoot
try {
    foreach ($command in @('node', 'npm', 'uv', 'python', 'git')) {
        if (-not (Get-Command $command -ErrorAction SilentlyContinue)) {
            throw "Missing prerequisite: $command. See docs/bmad/README.md."
        }
    }
    & npm ci --prefix $toolRoot --ignore-scripts --no-audit --no-fund
    if ($LASTEXITCODE -ne 0) { throw 'BMad npm ci failed' }
    $installer = Join-Path $toolRoot 'node_modules/bmad-method/tools/installer/bmad-cli.js'
    $actionArguments = @()
    if (Test-Path -LiteralPath (Join-Path $projectRoot '_bmad/_config/manifest.yaml')) {
        $actionArguments = @('--action', 'update')
    }
    & node $installer install --directory $projectRoot --modules gds --tools $versions.tools --pin "gds=$($versions.gds_tag)" --channel stable --communication-language Russian --document-output-language Russian --user-name bogor --output-folder vault --set core.project_name=Rat-Expedition --set gds.project_name=Rat-Expedition --set gds.planning_artifacts=vault/game --set gds.implementation_artifacts=docs/bmad/generated --set gds.project_knowledge=docs --no-shims @actionArguments --yes
    if ($LASTEXITCODE -ne 0) { throw 'BMad installer failed' }
    & python scripts/bmad_vault.py configure
    if ($LASTEXITCODE -ne 0) { throw 'BMad project configuration failed' }
    & python scripts/bmad_vault.py smoke
    if ($LASTEXITCODE -ne 0) { throw 'BMad smoke checks failed' }
}
finally { Pop-Location }
