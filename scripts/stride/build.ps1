[CmdletBinding()]
param([string]$CheckoutPath)

. (Join-Path $PSScriptRoot 'common.ps1')
try {
    $root = Get-StrideCheckoutPath $CheckoutPath
    $head = Assert-StrideCheckout $root
    Assert-StrideLfs $root
    Get-Command dotnet -ErrorAction Stop | Out-Null
    Push-Location $root
    try {
        $sdk = & dotnet --version
        if ($LASTEXITCODE -ne 0) { throw 'Install an SDK accepted by the locked upstream global.json.' }
        $logDir = Join-Path $root ('logs/rat-foundation/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
        New-Item -ItemType Directory -Path $logDir | Out-Null
        $log = Join-Path $logDir 'build.log'
        $binlog = Join-Path $logDir 'build.binlog'
        $arguments = @('build', $script:StrideLock.editorProject, '-c', $script:StrideLock.configuration,
            "-p:StridePlatforms=$($script:StrideLock.platforms)", "-p:StrideGraphicsApis=$($script:StrideLock.graphicsApis)",
            "-p:StrideNativeWindowsArm64Enabled=$($script:StrideLock.windowsArm64Enabled.ToString().ToLowerInvariant())",
            "-bl:$binlog")
        Write-Host "Building Stride $head with SDK $sdk. Log: $log"
        $previousPreference = $ErrorActionPreference
        $ErrorActionPreference = 'Continue'
        try {
            & dotnet @arguments *> $log
            $buildExit = $LASTEXITCODE
        } finally { $ErrorActionPreference = $previousPreference }
        $editor = Join-Path $root $script:StrideLock.editorExecutable
        $editorExists = Test-Path -LiteralPath $editor -PathType Leaf
        $editorVersion = $null
        if ($buildExit -eq 0 -and $editorExists) {
            $editorVersion = (Get-Item -LiteralPath $editor).VersionInfo.ProductVersion
        }
        $record = [ordered]@{ upstreamCommit = $script:StrideLock.upstreamCommit; head = $head; sdk = [string]$sdk
            arguments = $arguments; exitCode = $buildExit; log = $log; binaryLog = $binlog
            editor = $editor; editorExists = $editorExists; editorVersion = $editorVersion }
        $record | ConvertTo-Json -Depth 4 | Set-Content -Encoding UTF8 -LiteralPath (Join-Path $logDir 'result.json')
        Get-Content -LiteralPath $log -Tail 20 | Write-Host
        if ($buildExit -ne 0) { throw "Stride build failed (exit $buildExit). See $log" }
        if (-not $editorExists) { throw "Build returned success but the expected editor is absent: $editor" }
        Write-Host "Stride Release editor: $editor ($editorVersion). Evidence: $logDir"
    } finally { Pop-Location }
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
