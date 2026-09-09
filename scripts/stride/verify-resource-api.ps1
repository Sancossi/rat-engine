[CmdletBinding()]
param([string]$CheckoutPath,[string]$McpPython='python')
. (Join-Path $PSScriptRoot 'common.ps1')
$engine=Get-StrideCheckoutPath $CheckoutPath
$editorBin=Join-Path $engine 'sources/editor/Stride.GameStudio/bin/Release/net10.0-windows'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$authoring=Join-Path $repository 'games/rat-expedition/Rat.Expedition.Authoring'
$fixturePaths=@((Join-Path $authoring 'Assets/McpResourceQualification'),(Join-Path $authoring 'Resources/McpResourceQualification'))
foreach($path in $fixturePaths){if(Test-Path -LiteralPath $path){throw "Refusing to overwrite existing fixture folder: $path"}}
$output=Join-Path $repository ('build/mcp/resource-api-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $output | Out-Null
$result=Join-Path $output 'result.json'
& (Join-Path $PSScriptRoot 'build-mcp.ps1') -CheckoutPath $CheckoutPath
if($LASTEXITCODE -ne 0){throw 'MCP build failed.'}
& dotnet build (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification') -c Release -p:RestoreLockedMode=true "-p:StrideBin=$editorBin" --nologo
if($LASTEXITCODE -ne 0){throw 'Resource API qualification build failed.'}
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification/bin/Release/net10.0-windows/Rat.StrideMcp.Qualification.dll') -Destination (Join-Path $repository 'build/stride-mcp/adapter')
$launch=@(& (Join-Path $PSScriptRoot 'start-mcp-editor.ps1') -CheckoutPath $CheckoutPath -SolutionPath (Join-Path $repository 'games/rat-expedition/Rat.Expedition.sln') -QualificationResult $result -QualificationMode resources -PassThru) | Where-Object { $_.PSObject.Properties['OwnedProcess'] }
if(@($launch).Count -ne 1){throw 'Launcher did not return exactly one owned process.'}
$connectionPath=$launch.ConnectionPath
$connection=Get-Content -LiteralPath $connectionPath -Raw | ConvertFrom-Json
$owned=$launch.OwnedProcess
if($connection.processId -ne $owned.Id){throw 'This launch descriptor does not match its process.'}
try{
    $deadline=[DateTime]::UtcNow.AddSeconds(60)
    while(-not(Test-Path -LiteralPath ($result+'.ready.json'))){
        if(Test-Path -LiteralPath $result){throw (Get-Content -LiteralPath $result -Raw)}
        if($owned.HasExited -or [DateTime]::UtcNow -ge $deadline){throw 'Resource fixture did not become ready within 60 seconds.'}
        Start-Sleep -Milliseconds 200
    }
    & $McpPython (Join-Path $repository 'tools/stride-mcp/verify_resources.py') --server (Join-Path $repository 'build/stride-mcp/server/Rat.StrideMcp.Server.exe') --connection $connectionPath --fixture ($result+'.ready.json') --output ($result+'.client.json')
    if($LASTEXITCODE -ne 0){throw "Official MCP client failed; see $result.client.json"}
    $deadline=[DateTime]::UtcNow.AddSeconds(40)
    while(-not(Test-Path -LiteralPath $result)){
        if($owned.HasExited -or [DateTime]::UtcNow -ge $deadline){throw 'Native source-update fixture exceeded 40 seconds.'}
        Start-Sleep -Milliseconds 200
    }
    $evidence=Get-Content -LiteralPath $result -Raw | ConvertFrom-Json
    if(-not $evidence.passed){throw "Native resource qualification failed: $($evidence.error)"}
    Write-Host "Resource MCP and native source-update qualification PASS: $result"
}finally{
    # Exact opt-in process deliberately calls native Destroy. Do not run normal
    # GameStudio close handlers against the destroyed session a second time.
    if(-not $owned.HasExited){
        if($owned.StartTime.ToUniversalTime() -ne $launch.StartTimeUtc -or $owned.MainModule.FileName -ne $launch.ExecutablePath){throw 'Owned process identity changed; refusing termination.'}
        $owned.Kill();if(-not $owned.WaitForExit(10000)){throw 'Owned resource editor did not exit within 10 seconds.'}
    }
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
