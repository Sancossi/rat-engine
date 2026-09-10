[CmdletBinding()]
param([string]$CheckoutPath,[string]$McpPython='python')
. (Join-Path $PSScriptRoot 'common.ps1')
. (Join-Path $PSScriptRoot 'mcp-process.ps1')
$ErrorActionPreference='Stop'
$engine=Get-StrideCheckoutPath $CheckoutPath
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output=Join-Path $repository ('build/mcp/dremma-scene-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $output | Out-Null
$server=Join-Path $output 'server'
& (Join-Path $PSScriptRoot 'build-mcp.ps1') -CheckoutPath $CheckoutPath -ServerOutputPath $server
if($LASTEXITCODE -ne 0){throw 'MCP build failed.'}
& dotnet build (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification') -c Release -p:RestoreLockedMode=true "-p:StrideBin=$engine/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows" --nologo
if($LASTEXITCODE -ne 0){throw 'Dremma scene qualification build failed.'}
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification/bin/Release/net10.0-windows/Rat.StrideMcp.Qualification.dll') -Destination (Join-Path $repository 'build/stride-mcp/adapter')
$owner=@{}
try {
    foreach($phase in @('dremma-scene','dremma-scene-reopen')) {
        $result=Join-Path $output $(if($phase -eq 'dremma-scene'){'result.json'}else{'reopen.json'})
        & (Join-Path $PSScriptRoot 'start-mcp-editor.ps1') -CheckoutPath $CheckoutPath -SolutionPath (Join-Path $repository 'games/rat-expedition/Rat.Expedition.sln') -QualificationResult $result -QualificationMode $phase -LaunchOwnership $owner
        $deadline=[DateTime]::UtcNow.AddSeconds(60)
        while(-not(Test-Path -LiteralPath ($result+'.ready.json'))) {
            if(Test-Path -LiteralPath $result){throw (Get-Content -LiteralPath $result -Raw)}
            if($owner.OwnedProcess.HasExited -or [DateTime]::UtcNow -ge $deadline){throw "Dremma scene $phase readiness exceeded 60 seconds."}
            Start-Sleep -Milliseconds 150
        }
        $arguments=@((Join-Path $repository 'tools/stride-mcp/verify_dremma_scene.py'),'--server',(Join-Path $server 'Rat.StrideMcp.Server.exe'),'--connection',$owner.ConnectionPath,'--ready',($result+'.ready.json'),'--output',($result+'.client.json'))
        if($phase -eq 'dremma-scene-reopen'){$arguments+=@('--baseline',(Join-Path $output 'result.json.client.json'))}
        & $McpPython @arguments
        if($LASTEXITCODE -ne 0){throw "Dremma scene MCP client failed in $phase."}
        $deadline=[DateTime]::UtcNow.AddSeconds(30)
        while(-not(Test-Path -LiteralPath $result)) {
            if($owner.OwnedProcess.HasExited -or [DateTime]::UtcNow -ge $deadline){throw "Dremma scene $phase completion exceeded 30 seconds."}
            Start-Sleep -Milliseconds 150
        }
        $qualified=Get-Content -LiteralPath $result -Raw | ConvertFrom-Json
        if(-not $qualified.passed){throw "Dremma scene $phase failed: $($qualified.error)"}
        Stop-OwnedMcpProcess $owner;$owner=@{}
    }
    Write-Host "Current-editor Dremma edit/save/fresh-reopen PASS: $output"
} finally {Stop-OwnedMcpProcess $owner}
