[CmdletBinding()]
param([string]$CheckoutPath)

. (Join-Path $PSScriptRoot 'common.ps1')
try {
    $engine = Get-StrideCheckoutPath $CheckoutPath
    $engineHead = Assert-StrideCheckout $engine
    if ($engineHead -ne (Get-StrideIntegrationCommit)) { throw 'Authoring qualification requires the exact locked engine integration build.' }
    $repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
    $game = Join-Path $repository 'games/rat-expedition'
    $feed = Join-Path $engine 'bin/packages'
    foreach ($package in @('Stride.Engine', 'Stride.AssetCompiler')) {
        if (-not (Test-Path -LiteralPath (Join-Path $feed "$package.4.4.0-dev.nupkg"))) { throw "Missing pinned local package $package.4.4.0-dev." }
    }
    $artifacts = Join-Path $repository ('build/stride-authoring/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    $publish = Join-Path $artifacts 'publish'
    New-Item -ItemType Directory -Path $artifacts | Out-Null
    [xml]$config = Get-Content -LiteralPath (Join-Path $game 'NuGet.Config') -Raw
    ($config.configuration.packageSources.add | Where-Object key -eq 'stride-local').SetAttribute('value', [string]$feed)
    ($config.configuration.config.add | Where-Object key -eq 'globalPackagesFolder').SetAttribute('value', [string](Join-Path $game '.packages'))
    $configPath = Join-Path $artifacts 'NuGet.Config'
    $config.Save($configPath)
    function Invoke-AuthoringDotnet([string]$Name, [string[]]$Arguments) {
        $log = Join-Path $artifacts "$Name.log"
        $previousPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try { & dotnet @Arguments *>&1 | Out-File -LiteralPath $log -Encoding UTF8; $code = $LASTEXITCODE }
        finally { $ErrorActionPreference = $previousPreference }
        Get-Content -LiteralPath $log -Tail 6 | Write-Host
        if ($code -ne 0) { throw "$Name failed with exit $code. See $log" }
    }
    Push-Location -LiteralPath $game
    try {
        $app = 'Rat.Expedition.Authoring.Windows/Rat.Expedition.Authoring.Windows.csproj'
        Invoke-AuthoringDotnet 'restore' @('restore', $app, '--configfile', $configPath, '--locked-mode')
        Invoke-AuthoringDotnet 'publish' @('publish', $app, '-c', 'Release', '-r', 'win-x64', '--self-contained', 'true', '--no-restore', '-o', $publish)
    } finally { Pop-Location }
    $executable = Join-Path $publish 'Rat.Expedition.Authoring.Windows.exe'
    foreach ($relative in @('Rat.Expedition.Authoring.Windows.exe', 'coreclr.dll', 'data/db/bundles/default.bundle')) {
        if (-not (Test-Path -LiteralPath (Join-Path $publish $relative))) { throw "Native publish missing $relative" }
    }
    # Qualification artifact, not the distributable P1 ZIP. Keep upstream notices with it.
    $licenses = New-Item -ItemType Directory -Path (Join-Path $publish 'licenses')
    Copy-Item -LiteralPath (Join-Path $repository 'LICENSE') -Destination (Join-Path $licenses.FullName 'Rat-Expedition-LICENSE')
    Copy-Item -LiteralPath (Join-Path $engine 'LICENSE.md'),(Join-Path $engine 'THIRD PARTY.md') -Destination $licenses.FullName
    $evidence = Join-Path $artifacts 'runtime'
    # Paths cannot contain quotes on Windows; quote the one explicit path argument.
    $process = Start-Process -FilePath $executable -ArgumentList @('--smoke-frames', '60', '--evidence-dir', ('"' + $evidence + '"')) -WorkingDirectory ([IO.Path]::GetTempPath()) -WindowStyle Hidden -PassThru
    if (-not $process.WaitForExit(30000)) {
        $process.Kill(); $process.WaitForExit()
        throw 'Own authoring qualification process exceeded 30 seconds.'
    }
    if ($process.ExitCode -ne 0) { throw "Authoring runtime failed with exit $($process.ExitCode). See $evidence" }
    foreach ($file in @('loaded-asset.json', 'native-scene.png')) {
        if (-not (Test-Path -LiteralPath (Join-Path $evidence $file))) { throw "Runtime evidence missing $file" }
    }
    [ordered]@{
        exitCode = 0; sourceBaseline = $script:StrideLock.upstreamCommit; engineHead = $engineHead
        gameCommit = [string](Invoke-StrideGit $repository @('rev-parse', 'HEAD'))
        authoringWorkingTreeDirty = [bool](Invoke-StrideGit $repository @('status', '--porcelain', '--', 'games/rat-expedition/Rat.Expedition.Authoring', 'games/rat-expedition/Rat.Expedition.Authoring.Windows', 'games/rat-expedition/Rat.Expedition.Authoring.sln', 'scripts/stride/build-authoring.ps1'))
        executable = $executable; artifacts = $artifacts; evidence = $evidence
        runtimeWorkingDirectory = [IO.Path]::GetTempPath(); stridePackageVersion = '4.4.0-dev'
    } | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $artifacts 'result.json') -Encoding UTF8
    Write-Host "Native authoring qualification: $artifacts"
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
