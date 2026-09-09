[CmdletBinding()]
param([string]$CheckoutPath,[string]$McpPython='python')
. (Join-Path $PSScriptRoot 'common.ps1')
. (Join-Path $PSScriptRoot 'mcp-process.ps1')
$engine=Get-StrideCheckoutPath $CheckoutPath
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$fixture=[IO.Path]::GetFullPath((Join-Path $repository 'games/rat-expedition/Rat.Expedition.Authoring/Assets/McpActionsQualification'))
if(Test-Path -LiteralPath $fixture){throw 'Own action fixture already exists.'}
$output=Join-Path $repository ('build/mcp/asset-actions-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $output | Out-Null
$server=Join-Path $output 'server'
& (Join-Path $PSScriptRoot 'build-mcp.ps1') -CheckoutPath $CheckoutPath -ServerOutputPath $server
& dotnet build (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification') -c Release -p:RestoreLockedMode=true "-p:StrideBin=$engine/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows" --nologo
if($LASTEXITCODE -ne 0){throw 'Action fixture build failed.'}
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification/bin/Release/net10.0-windows/Rat.StrideMcp.Qualification.dll') -Destination (Join-Path $repository 'build/stride-mcp/adapter')
$owner=@{}
try{
    foreach($phase in @('actions','actions-reopen')){
        $result=Join-Path $output $(if($phase -eq 'actions'){'result.json'}else{'reopen.json'})
        & (Join-Path $PSScriptRoot 'start-mcp-editor.ps1') -CheckoutPath $CheckoutPath -SolutionPath (Join-Path $repository 'games/rat-expedition/Rat.Expedition.sln') -QualificationResult $result -QualificationMode $phase -LaunchOwnership $owner
        $deadline=[DateTime]::UtcNow.AddSeconds(45)
        while(-not(Test-Path -LiteralPath ($result+'.ready.json'))){
            if(Test-Path -LiteralPath $result){throw (Get-Content -LiteralPath $result -Raw)}
            if($owner.OwnedProcess.HasExited -or [DateTime]::UtcNow -ge $deadline){throw 'Action fixture readiness exceeded deadline.'}
            Start-Sleep -Milliseconds 150
        }
        $argsClient=@((Join-Path $repository 'tools/stride-mcp/verify_asset_actions.py'),'--server',(Join-Path $server 'Rat.StrideMcp.Server.exe'),'--connection',$owner.ConnectionPath,'--fixture',($result+'.ready.json'),'--output',($result+'.client.json'),'--snapshot',(Join-Path $output 'saved-snapshot.json'))
        if($phase -eq 'actions-reopen'){$argsClient+='--reopen'}
        & $McpPython @argsClient
        if($LASTEXITCODE -ne 0){throw 'MCP action client failed.'}
        $deadline=[DateTime]::UtcNow.AddSeconds(20)
        while(-not(Test-Path -LiteralPath $result)){
            if($owner.OwnedProcess.HasExited -or [DateTime]::UtcNow -ge $deadline){throw 'Action fixture completion exceeded deadline.'}
            Start-Sleep -Milliseconds 150
        }
        $qualified=Get-Content -LiteralPath $result -Raw | ConvertFrom-Json
        if(-not $qualified.passed){throw ('Native action qualification failed: '+$qualified.error)}
        Stop-OwnedMcpProcess $owner;$owner=@{}
    }
    Write-Host "Native MCP actions and fresh reopen PASS: $output"
}finally{
    Stop-OwnedMcpProcess $owner
    if(Test-Path -LiteralPath $fixture){
        $target=[IO.Path]::GetFullPath((Join-Path $output 'saved-fixture'))
        if(-not $fixture.StartsWith($repository+[IO.Path]::DirectorySeparatorChar) -or -not $target.StartsWith($output+[IO.Path]::DirectorySeparatorChar)){throw 'Fixture move escaped owned roots.'}
        if(@(Get-Item -LiteralPath $fixture)+@(Get-ChildItem -LiteralPath $fixture -Recurse -Force) | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}){throw 'Reparse point in action fixture; refusing move.'}
        Move-Item -LiteralPath $fixture -Destination $target
    }
}
