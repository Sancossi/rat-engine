[CmdletBinding()]
param([string]$CheckoutPath,[string]$McpPython='python',[switch]$FailedReadiness)
. (Join-Path $PSScriptRoot 'common.ps1')
. (Join-Path $PSScriptRoot 'mcp-process.ps1')
$engine=Get-StrideCheckoutPath $CheckoutPath
$editorBin=Join-Path $engine 'sources/editor/Stride.GameStudio/bin/Release/net10.0-windows'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$authoring=Join-Path $repository 'games/rat-expedition/Rat.Expedition.Authoring'
$fixturePaths=@((Join-Path $authoring 'Assets/McpResourceQualification'),(Join-Path $authoring 'Resources/McpResourceQualification'))
foreach($path in $fixturePaths){if(Test-Path -LiteralPath $path){throw "Refusing to overwrite existing fixture folder: $path"}}
$output=Join-Path $repository ('build/mcp/resource-api-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $output | Out-Null
$result=Join-Path $output 'result.json'
$serverOutput=Join-Path $output 'server'
& (Join-Path $PSScriptRoot 'build-mcp.ps1') -CheckoutPath $CheckoutPath -ServerOutputPath $serverOutput
if($LASTEXITCODE -ne 0){throw 'MCP build failed.'}
& dotnet build (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification') -c Release -p:RestoreLockedMode=true "-p:StrideBin=$editorBin" --nologo
if($LASTEXITCODE -ne 0){throw 'Resource API qualification build failed.'}
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification/bin/Release/net10.0-windows/Rat.StrideMcp.Qualification.dll') -Destination (Join-Path $repository 'build/stride-mcp/adapter')
$launch=@{};$sentinel=$null;$failedReadinessEvidence=$null
try{
    $mode='resources';$timeout=60
    if($FailedReadiness){
        $mode='readiness-failure';$timeout=5
        $sentinel=Start-Process powershell -ArgumentList '-NoProfile -Command "Start-Sleep -Seconds 60"' -WindowStyle Hidden -PassThru
    }
    try{
        & (Join-Path $PSScriptRoot 'start-mcp-editor.ps1') -CheckoutPath $CheckoutPath -SolutionPath (Join-Path $repository 'games/rat-expedition/Rat.Expedition.sln') -QualificationResult $result -QualificationMode $mode -LaunchOwnership $launch -ReadinessTimeoutSeconds $timeout
    }catch{
        if(-not $FailedReadiness -or $_.Exception.Message -notlike 'Adapter readiness exceeded*'){throw}
        if($null -eq $launch.OwnedProcess -or -not $launch.OwnedProcess.HasExited){throw 'Failed readiness leaked its owned editor.'}
        if(-not(Test-Path -LiteralPath ($result+'.failure-started.json'))){throw 'Controlled startup fixture did not run.'}
        if($sentinel.HasExited){throw 'Other owned sentinel process was affected by readiness cleanup.'}
        $failedReadinessEvidence=@{passed=$true;failedReadiness=$true;editorPid=$launch.OwnedProcess.Id;editorExited=$true;sentinelPid=$sentinel.Id;sentinelUnaffected=$true}
    }
    if($FailedReadiness){
        if($null -eq $failedReadinessEvidence){throw 'Readiness failure qualification unexpectedly succeeded.'}
    }else{
    $connectionPath=$launch.ConnectionPath
    $connection=Get-Content -LiteralPath $connectionPath -Raw | ConvertFrom-Json
    $owned=$launch.OwnedProcess
    if($connection.processId -ne $owned.Id){throw 'This launch descriptor does not match its process.'}
    $deadline=[DateTime]::UtcNow.AddSeconds(60)
    while(-not(Test-Path -LiteralPath ($result+'.ready.json'))){
        if(Test-Path -LiteralPath $result){throw (Get-Content -LiteralPath $result -Raw)}
        if($owned.HasExited -or [DateTime]::UtcNow -ge $deadline){throw 'Resource fixture did not become ready within 60 seconds.'}
        Start-Sleep -Milliseconds 200
    }
    & $McpPython (Join-Path $repository 'tools/stride-mcp/verify_resources.py') --server (Join-Path $serverOutput 'Rat.StrideMcp.Server.exe') --connection $connectionPath --fixture ($result+'.ready.json') --output ($result+'.client.json')
    if($LASTEXITCODE -ne 0){throw "Official MCP client failed; see $result.client.json"}
    $deadline=[DateTime]::UtcNow.AddSeconds(40)
    while(-not(Test-Path -LiteralPath $result)){
        if($owned.HasExited -or [DateTime]::UtcNow -ge $deadline){throw 'Native source-update fixture exceeded 40 seconds.'}
        Start-Sleep -Milliseconds 200
    }
    $evidence=Get-Content -LiteralPath $result -Raw | ConvertFrom-Json
    if(-not $evidence.passed){throw "Native resource qualification failed: $($evidence.error)"}
    Write-Host "Resource MCP and native source-update qualification PASS: $result"
    }
}finally{
    # Exact opt-in process deliberately calls native Destroy. Do not run normal
    # GameStudio close handlers against the destroyed session a second time.
    Stop-OwnedMcpProcess $launch
    if($null -ne $sentinel -and -not $sentinel.HasExited){$sentinel.Kill();if(-not $sentinel.WaitForExit(5000)){throw 'Owned sentinel did not stop.'}}
    # Keep native saved files as evidence, away from the normal asset build.
    # Both source folders were absent before this owned test session started.
    for($i=0;$i -lt $fixturePaths.Count;$i++){
        $source=[IO.Path]::GetFullPath($fixturePaths[$i])
        $target=[IO.Path]::GetFullPath((Join-Path $output ('saved-fixture-'+$i)))
        if(-not $source.StartsWith($authoring+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or -not $target.StartsWith($output+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture move escaped its owned roots.'}
        if(Test-Path -LiteralPath $source){
            $entries=@(Get-Item -LiteralPath $source)+@(Get-ChildItem -LiteralPath $source -Recurse -Force)
            if($entries | Where-Object { $_.Attributes -band [IO.FileAttributes]::ReparsePoint }){throw 'Refusing to move fixture containing a reparse point.'}
            Move-Item -LiteralPath $source -Destination $target
        }
    }
}
if($null -ne $failedReadinessEvidence){
    foreach($i in 0,1){
        if(-not(Test-Path -LiteralPath (Join-Path $output "saved-fixture-$i/failed-readiness.txt")) -or (Test-Path -LiteralPath $fixturePaths[$i])){throw 'Failed-readiness fixture evidence was lost or left in production assets.'}
    }
    $failedReadinessEvidence.fixturesPreservedAndRemoved=$true
    $failedReadinessEvidence | ConvertTo-Json | Set-Content -LiteralPath $result -Encoding UTF8
    Write-Host "Owned failed-readiness qualification PASS: $result"
}
