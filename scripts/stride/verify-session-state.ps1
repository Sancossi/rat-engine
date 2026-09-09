[CmdletBinding()]
param([string]$CheckoutPath)
. (Join-Path $PSScriptRoot 'common.ps1')
$engine=Get-StrideCheckoutPath $CheckoutPath
$editorBin=Join-Path $engine 'sources/editor/Stride.GameStudio/bin/Release/net10.0-windows'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output=Join-Path $repository ('build/mcp/session-state-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $output | Out-Null
$result=Join-Path $output 'result.json'
& (Join-Path $PSScriptRoot 'build-mcp.ps1') -CheckoutPath $CheckoutPath -ServerOutputPath (Join-Path $output 'server')
if($LASTEXITCODE -ne 0){throw 'MCP build failed.'}
& dotnet build (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification/Rat.StrideMcp.Qualification.csproj') -c Release -p:RestoreLockedMode=true "-p:StrideBin=$editorBin" --nologo
if($LASTEXITCODE -ne 0){throw 'Native session-state qualification build failed.'}
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification/bin/Release/net10.0-windows/Rat.StrideMcp.Qualification.dll') -Destination (Join-Path $repository 'build/stride-mcp/adapter')
& (Join-Path $PSScriptRoot 'start-mcp-editor.ps1') -CheckoutPath $CheckoutPath -QualificationResult $result
$connection=Get-Content -LiteralPath (Join-Path $repository 'build/stride-mcp/connection.json') -Raw | ConvertFrom-Json
$owned=Get-Process -Id $connection.processId
try{
    $deadline=[DateTime]::UtcNow.AddSeconds(60)
    while(-not(Test-Path -LiteralPath $result)){
        if($owned.HasExited){throw 'Qualification editor exited before evidence.'}
        if([DateTime]::UtcNow -ge $deadline){throw 'Native session-state qualification exceeded 60 seconds.'}
        Start-Sleep -Milliseconds 200
    }
    $evidence=Get-Content -LiteralPath $result -Raw | ConvertFrom-Json
    if(-not $evidence.passed){throw "Native session-state regression failed: $($evidence.error)"}
    Write-Host "Native session-state qualification PASS: $result"
}finally{
    # This exact owned fixture has deliberately destroyed its native session.
    # Avoid invoking GameStudio close handlers that would destroy it a second time.
    if(-not $owned.HasExited){$owned.Kill();$owned.WaitForExit()}
}
