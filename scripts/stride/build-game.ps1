[CmdletBinding()]
param([string]$CheckoutPath)

. (Join-Path $PSScriptRoot 'common.ps1')
try {
    $engine = Get-StrideCheckoutPath $CheckoutPath
    $engineHead = Assert-StrideCheckout $engine
    if ($engineHead -ne $script:StrideLock.upstreamCommit) {
        throw 'This game slice uses exactly the locked upstream build. Qualify custom engine packages in a separate slice.'
    }
    $repository = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
    $game = Join-Path $repository 'games/rat-expedition'
    $feed = Join-Path $engine 'bin/packages'
    foreach ($package in @('Stride.Engine', 'Stride.AssetCompiler')) {
        if (-not (Test-Path -LiteralPath (Join-Path $feed "$package.4.4.0-dev.nupkg"))) {
            throw "Missing pinned local package $package.4.4.0-dev. Build the locked engine first."
        }
    }
    $stamp = Get-Date -Format 'yyyyMMdd-HHmmss-fff'
    $artifacts = Join-Path $repository "build/stride-game/$stamp"
    $publish = Join-Path $artifacts 'publish'
    New-Item -ItemType Directory -Path $artifacts | Out-Null
    # Keep source mapping and a game-local cache, including when the engine is relocated.
    [xml]$config = Get-Content -LiteralPath (Join-Path $game 'NuGet.Config') -Raw
    ($config.configuration.packageSources.add | Where-Object key -eq 'stride-local').SetAttribute('value', [string]$feed)
    $cache = Join-Path $game '.packages'
    ($config.configuration.config.add | Where-Object key -eq 'globalPackagesFolder').SetAttribute('value', [string]$cache)
    $configPath = Join-Path $artifacts 'NuGet.Config'
    $config.Save($configPath)

    function Invoke-GameDotnet {
        param([string]$Name, [string[]]$Arguments)
        $log = Join-Path $artifacts "$Name.log"
        Write-Host "dotnet $($Arguments -join ' ') | log $log"
        $previousPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try { & dotnet @Arguments *>&1 | Out-File -LiteralPath $log -Encoding UTF8; $code = $LASTEXITCODE }
        finally { $ErrorActionPreference = $previousPreference }
        Get-Content -LiteralPath $log -Tail 6 | Write-Host
        if ($code -ne 0) { throw "$Name failed with exit $code. See $log" }
    }

    Push-Location -LiteralPath $game
    try {
        $sdk = & dotnet --version
        if ($LASTEXITCODE -ne 0) { throw 'Install the SDK in games/rat-expedition/global.json.' }
        $app = 'Rat.Expedition.Windows/Rat.Expedition.Windows.csproj'
        $tests = 'Rat.Expedition.Core.Tests/Rat.Expedition.Core.Tests.csproj'
        Invoke-GameDotnet 'restore' @('restore', $app, '--configfile', $configPath, '--locked-mode')
        Invoke-GameDotnet 'restore-tests' @('restore', $tests, '--configfile', $configPath, '--locked-mode')
        Invoke-GameDotnet 'core-tests' @('run', '--project', $tests, '-c', 'Release', '--no-restore')
        Invoke-GameDotnet 'publish' @('publish', $app, '-c', 'Release', '-r', 'win-x64', '--self-contained', 'true', '--no-restore', '-o', $publish,
            '-p:PublishAot=false', '-p:PublishTrimmed=false', '-p:PublishSingleFile=false')
    } finally { Pop-Location }

    foreach ($relative in @('Rat.Expedition.Windows.exe', 'coreclr.dll', 'Content/sprites/rat.png', 'Content/courtyard.json', 'Content/credits.json', 'data/db/bundles/default.bundle')) {
        if (-not (Test-Path -LiteralPath (Join-Path $publish $relative))) { throw "Publish closure missing $relative" }
    }
    $licenses = Join-Path $publish 'licenses'
    New-Item -ItemType Directory -Path $licenses | Out-Null
    Copy-Item -LiteralPath (Join-Path $repository 'LICENSE') -Destination (Join-Path $licenses 'Rat-Expedition-LICENSE')
    Copy-Item -LiteralPath (Join-Path $engine 'LICENSE.md') -Destination (Join-Path $licenses 'Stride-LICENSE.md')
    Copy-Item -LiteralPath (Join-Path $engine 'THIRD PARTY.md') -Destination (Join-Path $licenses 'Stride-THIRD-PARTY.md')
    $inventory = @()
    foreach ($nuspec in Get-ChildItem -LiteralPath $cache -Filter '*.nuspec' -Recurse -File) {
        [xml]$metadata = Get-Content -LiteralPath $nuspec.FullName -Raw
        $package = $metadata.package.metadata
        $licenseNode = $package.SelectSingleNode("*[local-name()='license']")
        $inventory += [ordered]@{ id = [string]$package.id; version = [string]$package.version; license = $(if ($licenseNode) { $licenseNode.InnerText } else { 'See package notices and upstream inventory' }) }
        $texts = @(Get-ChildItem -LiteralPath $nuspec.DirectoryName -Recurse -File | Where-Object { $_.Name -match '^(LICENSE|NOTICE|COPYING|THIRD.?PARTY)([._ -]|$)' })
        if ($texts.Count) {
            $destination = Join-Path $licenses "$($package.id)/$($package.version)"
            foreach ($license in $texts) {
                $relative = $license.FullName.Substring($nuspec.DirectoryName.Length).TrimStart('\', '/')
                $target = Join-Path $destination $relative
                [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($target)) | Out-Null
                Copy-Item -LiteralPath $license.FullName -Destination $target
            }
        }
    }
    $inventory | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $licenses 'package-inventory.json') -Encoding UTF8
    $gameCommit = [string](Invoke-StrideGit $repository @('rev-parse', 'HEAD'))
    $lockPaths = @(Get-ChildItem -LiteralPath $game -Filter 'packages.lock.json' -Recurse -File | Where-Object { -not $_.FullName.StartsWith($cache + '\', [StringComparison]::OrdinalIgnoreCase) })
    $manifest = [ordered]@{
        sourceBaseline = $script:StrideLock.upstreamCommit; engineHead = $engineHead; gameCommit = $gameCommit
        gameWorkingTreeDirty = [bool](Invoke-StrideGit $repository @('status', '--porcelain', '--', 'games/rat-expedition', 'scripts/stride/build-game.ps1'))
        sdk = [string]$sdk; stridePackageVersion = '4.4.0-dev'; configuration = 'Release'; runtimeIdentifier = 'win-x64'
        selfContained = $true; effectCompiler = 'Local; remote feature switch disabled'; packageFeed = $feed
        locks = @($lockPaths | ForEach-Object { [ordered]@{ path = $_.FullName.Substring($game.Length); sha256 = (Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash } })
    }
    $manifest | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $publish 'build-manifest.json') -Encoding UTF8
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $zip = Join-Path $artifacts 'rat-expedition-0.1.0-win-x64.zip'
    [IO.Compression.ZipFile]::CreateFromDirectory($publish, $zip)
    [ordered]@{ exitCode = 0; editorUnchanged = $true; executable = (Join-Path $publish 'Rat.Expedition.Windows.exe'); zip = $zip; artifacts = $artifacts } |
        ConvertTo-Json | Set-Content -LiteralPath (Join-Path $artifacts 'result.json') -Encoding UTF8
    Write-Host "Game package: $zip"
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
