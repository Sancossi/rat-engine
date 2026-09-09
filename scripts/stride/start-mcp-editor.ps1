[CmdletBinding()]
param([string]$CheckoutPath,[string]$SolutionPath)
. (Join-Path $PSScriptRoot 'common.ps1')
$engine=Get-StrideCheckoutPath $CheckoutPath
if((Assert-StrideCheckout $engine) -ne $script:StrideLock.upstreamCommit){throw 'Exact pinned Stride is required.'}
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
if(-not $SolutionPath){$SolutionPath=Join-Path $repository 'games/rat-expedition/Rat.Expedition.Authoring.sln'}
$solution=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($SolutionPath)
if(-not(Test-Path -LiteralPath $solution -PathType Leaf)){throw 'Solution does not exist.'}
$hook=Join-Path $repository 'build/stride-mcp/adapter/Rat.StrideMcp.Hook.dll'
if(-not(Test-Path -LiteralPath $hook)){throw 'Run build-mcp.ps1 first.'}
# Fail before GUI startup when a mutable dev feed no longer matches the committed
# package cohort. Never open a project with unloadable custom components.
$preflight=Join-Path $repository 'build/stride-mcp/authoring-preflight.log'
& dotnet build $solution -c Debug --nologo -p:RestoreLockedMode=true *> $preflight
if($LASTEXITCODE -ne 0){throw "Native authoring preflight failed; editor was not started. See $preflight"}
$connectionDir=Join-Path $repository ('build/stride-mcp/sessions/'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $connectionDir | Out-Null
$connection=Join-Path $connectionDir 'connection.json'
$editor=Join-Path $engine 'sources/editor/Stride.GameStudio/bin/Release/net10.0-windows/Stride.GameStudio.exe'
$oldHook=$env:DOTNET_STARTUP_HOOKS;$oldConnection=$env:RAT_MCP_CONNECTION;$oldPipe=$env:RAT_MCP_PIPE
try {
    $env:DOTNET_STARTUP_HOOKS=$hook;$env:RAT_MCP_CONNECTION=$connection;$env:RAT_MCP_PIPE='rat-stride-mcp-'+[Guid]::NewGuid().ToString('N')
    $process=Start-Process -FilePath $editor -ArgumentList ('"'+$solution+'"') -WorkingDirectory (Split-Path $solution) -WindowStyle Hidden -PassThru
} finally {$env:DOTNET_STARTUP_HOOKS=$oldHook;$env:RAT_MCP_CONNECTION=$oldConnection;$env:RAT_MCP_PIPE=$oldPipe}
[ordered]@{processId=$process.Id;connection=$connection;solution=$solution} | ConvertTo-Json | Tee-Object -FilePath (Join-Path $connectionDir 'launch.json')
$deadline=[DateTime]::UtcNow.AddSeconds(60)
while(-not(Test-Path -LiteralPath $connection)){
    if($process.HasExited){throw "Own editor exited before adapter readiness. See $connectionDir"}
    if([DateTime]::UtcNow -ge $deadline){throw "Adapter readiness exceeded 60 seconds; inspect own PID $($process.Id)."}
    Start-Sleep -Milliseconds 200
}
$ready=Get-Content -LiteralPath $connection -Raw | ConvertFrom-Json
if($ready.processId -ne $process.Id){throw 'Descriptor PID differs from launched editor.'}
# Selection changes only at explicit launch. Already-running MCP servers keep
# their immutable descriptor snapshot and cannot silently retarget a new editor.
Copy-Item -LiteralPath $connection -Destination (Join-Path $repository 'build/stride-mcp/connection.json')
Write-Host "Ready: $connection"
