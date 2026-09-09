[CmdletBinding()]
param([string]$CheckoutPath)
. (Join-Path $PSScriptRoot 'common.ps1')
. (Join-Path $PSScriptRoot 'mcp-process.ps1')
$engine=Get-StrideCheckoutPath $CheckoutPath
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$fixture=[IO.Path]::GetFullPath((Join-Path $repository 'games/rat-expedition/Rat.Expedition.Authoring/Assets/McpCreationFailure'))
if(Test-Path -LiteralPath $fixture){throw 'Own creation fixture already exists.'}
$output=Join-Path $repository ('build/mcp/creation-failure-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $output | Out-Null
$result=Join-Path $output 'result.json'
& dotnet build (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification') -c Release -p:RestoreLockedMode=true "-p:StrideBin=$engine/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows" --nologo
if($LASTEXITCODE -ne 0){throw 'Creation probe build failed.'}
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification/bin/Release/net10.0-windows/Rat.StrideMcp.Qualification.dll') -Destination (Join-Path $repository 'build/stride-mcp/adapter')
$owner=@{}
try{
    & (Join-Path $PSScriptRoot 'start-mcp-editor.ps1') -CheckoutPath $CheckoutPath -SolutionPath (Join-Path $repository 'games/rat-expedition/Rat.Expedition.sln') -QualificationResult $result -QualificationMode creation-failure -LaunchOwnership $owner
    $deadline=[DateTime]::UtcNow.AddSeconds(30)
    while(-not(Test-Path -LiteralPath $result)){
        if($owner.OwnedProcess.HasExited -or [DateTime]::UtcNow -ge $deadline){throw 'Creation probe exceeded deadline.'}
        Start-Sleep -Milliseconds 200
    }
    $qualified=Get-Content -LiteralPath $result -Raw | ConvertFrom-Json
    if(-not $qualified.passed){throw ('Native creation qualification failed: '+$qualified.error)}
    Stop-OwnedMcpProcess $owner
    $owner=@{}
    $reopenResult=Join-Path $output 'reopen.json'
    & (Join-Path $PSScriptRoot 'start-mcp-editor.ps1') -CheckoutPath $CheckoutPath -SolutionPath (Join-Path $repository 'games/rat-expedition/Rat.Expedition.sln') -QualificationResult $reopenResult -QualificationMode creation-reopen -LaunchOwnership $owner
    $deadline=[DateTime]::UtcNow.AddSeconds(30)
    while(-not(Test-Path -LiteralPath $reopenResult)){
        if($owner.OwnedProcess.HasExited -or [DateTime]::UtcNow -ge $deadline){throw 'Creation reopen exceeded deadline.'}
        Start-Sleep -Milliseconds 200
    }
    $reopened=Get-Content -LiteralPath $reopenResult -Raw | ConvertFrom-Json
    if(-not $reopened.passed -or $reopened.reopenedAssetId -ne $qualified.successfulAssetId){throw ('Native reopen failed: '+$reopened.error)}
    Get-Content -LiteralPath $result -Raw
    Get-Content -LiteralPath $reopenResult -Raw
}finally{
    Stop-OwnedMcpProcess $owner
    if(Test-Path -LiteralPath $fixture){
        $target=[IO.Path]::GetFullPath((Join-Path $output 'saved-fixture'))
        if(-not $fixture.StartsWith($repository+[IO.Path]::DirectorySeparatorChar) -or -not $target.StartsWith($output+[IO.Path]::DirectorySeparatorChar)){throw 'Fixture move escaped owned roots.'}
        if(@(Get-Item -LiteralPath $fixture)+@(Get-ChildItem -LiteralPath $fixture -Recurse -Force) | Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}){throw 'Reparse point in creation fixture; refusing move.'}
        Move-Item -LiteralPath $fixture -Destination $target
    }
}
