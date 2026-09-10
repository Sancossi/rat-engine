[CmdletBinding()]
param([string]$CheckoutPath,[string]$McpPython='python')
. (Join-Path $PSScriptRoot 'common.ps1')
. (Join-Path $PSScriptRoot 'mcp-process.ps1')
$ErrorActionPreference='Stop'
$engine=Get-StrideCheckoutPath $CheckoutPath
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$authoring=Join-Path $repository 'games/rat-expedition/Rat.Expedition.Authoring'
$fixturePaths=@((Join-Path $authoring 'Assets/DremmaLibraryQualification'),(Join-Path $authoring 'Resources/DremmaLibraryQualification'))
foreach($path in $fixturePaths){if(Test-Path -LiteralPath $path){throw "Refusing to overwrite existing fixture folder: $path"}}
$libraryPaths=@((Join-Path $authoring 'Assets/CanalCity'),(Join-Path $authoring 'Resources/CanalCity'))
function Get-LibraryManifest {
    $entries=foreach($root in $libraryPaths){
        Get-ChildItem -LiteralPath $root -Recurse -File | Sort-Object FullName | ForEach-Object {
            [ordered]@{path=$_.FullName.Substring($authoring.Length+1).Replace('\','/');length=$_.Length;sha256=(Get-FileHash -LiteralPath $_.FullName -Algorithm SHA256).Hash.ToLowerInvariant()}
        }
    }
    return @($entries)
}
$before=@(Get-LibraryManifest)
if($before.Count -ne 170){throw "Expected 97 native assets plus 73 source/evidence files; found $($before.Count)."}
$output=Join-Path $repository ('build/mcp/dremma-library-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $output | Out-Null
$before | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $output 'library-before.json') -Encoding UTF8
$server=Join-Path $output 'server'
& (Join-Path $PSScriptRoot 'build-mcp.ps1') -CheckoutPath $CheckoutPath -ServerOutputPath $server
if($LASTEXITCODE -ne 0){throw 'MCP build failed.'}
& dotnet build (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification') -c Release -p:RestoreLockedMode=true "-p:StrideBin=$engine/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows" --nologo
if($LASTEXITCODE -ne 0){throw 'Dremma library qualification build failed.'}
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification/bin/Release/net10.0-windows/Rat.StrideMcp.Qualification.dll') -Destination (Join-Path $repository 'build/stride-mcp/adapter')
$owner=@{}
try {
    foreach($phase in @('dremma-library','dremma-library-reopen')){
        $result=Join-Path $output $(if($phase -eq 'dremma-library'){'result.json'}else{'reopen.json'})
        & (Join-Path $PSScriptRoot 'start-mcp-editor.ps1') -CheckoutPath $CheckoutPath -SolutionPath (Join-Path $repository 'games/rat-expedition/Rat.Expedition.sln') -QualificationResult $result -QualificationMode $phase -LaunchOwnership $owner
        $deadline=[DateTime]::UtcNow.AddSeconds(60)
        while(-not(Test-Path -LiteralPath ($result+'.ready.json'))){
            if(Test-Path -LiteralPath $result){throw (Get-Content -LiteralPath $result -Raw)}
            if($owner.OwnedProcess.HasExited -or [DateTime]::UtcNow -ge $deadline){throw "Dremma library $phase readiness exceeded 60 seconds."}
            Start-Sleep -Milliseconds 150
        }
        $arguments=@((Join-Path $repository 'tools/stride-mcp/verify_dremma_library.py'),'--server',(Join-Path $server 'Rat.StrideMcp.Server.exe'),'--connection',$owner.ConnectionPath,'--fixture',($result+'.ready.json'),'--output',($result+'.client.json'),'--edit-output',($result+'.edit.json'),'--reimport',(Join-Path $output 'result.json.reimport.ready.json'))
        if($phase -eq 'dremma-library-reopen'){$arguments+='--reopen'}
        & $McpPython @arguments
        if($LASTEXITCODE -ne 0){throw "Dremma library MCP client failed in $phase."}
        $deadline=[DateTime]::UtcNow.AddSeconds(30)
        while(-not(Test-Path -LiteralPath $result)){
            if($owner.OwnedProcess.HasExited -or [DateTime]::UtcNow -ge $deadline){throw "Dremma library $phase completion exceeded 30 seconds."}
            Start-Sleep -Milliseconds 150
        }
        $qualified=Get-Content -LiteralPath $result -Raw | ConvertFrom-Json
        if(-not $qualified.passed){throw "Dremma library $phase failed: $($qualified.error)"}
        Stop-OwnedMcpProcess $owner;$owner=@{}
    }
    $after=@(Get-LibraryManifest)
    $after | ConvertTo-Json -Depth 4 | Set-Content -LiteralPath (Join-Path $output 'library-after.json') -Encoding UTF8
    if(($before|ConvertTo-Json -Depth 4) -ne ($after|ConvertTo-Json -Depth 4)){throw 'Original CanalCity library files changed during owned-copy qualification.'}
    [ordered]@{passed=$true;files=$after.Count;nativeAssets=97;sourceAndEvidenceFiles=73;unchanged=$true} | ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'library-integrity.json') -Encoding UTF8
    Write-Host "Current-editor Dremma library edit/reimport/fresh-reopen PASS: $output"
}
finally {
    Stop-OwnedMcpProcess $owner
    for($i=0;$i -lt $fixturePaths.Count;$i++){
        $source=[IO.Path]::GetFullPath($fixturePaths[$i])
        $target=[IO.Path]::GetFullPath((Join-Path $output "saved-fixture-$i"))
        if(-not $source.StartsWith($authoring+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase) -or -not $target.StartsWith($output+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Fixture move escaped owned roots.'}
        if(Test-Path -LiteralPath $source){
            $entries=@(Get-Item -LiteralPath $source)+@(Get-ChildItem -LiteralPath $source -Recurse -Force)
            if($entries|Where-Object {$_.Attributes -band [IO.FileAttributes]::ReparsePoint}){throw 'Refusing to move a Dremma fixture containing a reparse point.'}
            Move-Item -LiteralPath $source -Destination $target
        }
    }
}
