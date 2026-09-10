[CmdletBinding()]
param([string]$CheckoutPath,[string]$McpPython='python',[switch]$ExerciseCleanupFailure)
. (Join-Path $PSScriptRoot 'common.ps1')
. (Join-Path $PSScriptRoot 'mcp-process.ps1')
$ErrorActionPreference='Stop'
$engine=Get-StrideCheckoutPath $CheckoutPath
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output=Join-Path $repository ('build/mcp/dremma-scene-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
New-Item -ItemType Directory -Path $output | Out-Null
$scene=[IO.Path]::GetFullPath((Join-Path $repository 'games/rat-expedition/Rat.Expedition.Authoring/Assets/Dremma.sdscene'))
$authoring=[IO.Path]::GetFullPath((Join-Path $repository 'games/rat-expedition/Rat.Expedition.Authoring'))
if(-not $scene.StartsWith($authoring+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw 'Dremma scene escaped Authoring root.'}
$backup=Join-Path $output 'Dremma.before.sdscene'
Copy-Item -LiteralPath $scene -Destination $backup
$initialHash=(Get-FileHash -LiteralPath $scene -Algorithm SHA256).Hash
$utf8=[Text.UTF8Encoding]::new($false)
$initialText=[IO.File]::ReadAllText($backup,$utf8)
$newline=$(if($initialText.Contains("`r`n")){"`r`n"}else{"`n"})
$originalWallBlock=@(
    '                Name: canal-wall',
    '                Components:',
    '                    4184f6b487b15c3193fde07ce66759a6: !TransformComponent',
    '                        Id: 64e96621-f27a-53fc-bf5f-7cf8a918e70c',
    '                        Position: {X: 0.0, Y: 0.0, Z: 13.0}'
) -join $newline
if($initialText.IndexOf($originalWallBlock,[StringComparison]::Ordinal) -lt 0 -or
   $initialText.IndexOf($originalWallBlock,[StringComparison]::Ordinal) -ne $initialText.LastIndexOf($originalWallBlock,[StringComparison]::Ordinal)) {
    throw 'Expected exactly one original canal-wall transform before launching the editor.'
}
$editedWallBlock=$originalWallBlock.Replace('Z: 13.0','Z: 12.9')
$expectedEditedText=$initialText.Replace($originalWallBlock,$editedWallBlock)
$sha256=[Security.Cryptography.SHA256]::Create()
try {$ownedEditedHash=([BitConverter]::ToString($sha256.ComputeHash($utf8.GetBytes($expectedEditedText)))).Replace('-','')}
finally {$sha256.Dispose()}
$server=Join-Path $output 'server'
& (Join-Path $PSScriptRoot 'build-mcp.ps1') -CheckoutPath $CheckoutPath -ServerOutputPath $server
if($LASTEXITCODE -ne 0){throw 'MCP build failed.'}
& dotnet build (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification') -c Release -p:RestoreLockedMode=true "-p:StrideBin=$engine/sources/editor/Stride.GameStudio/bin/Release/net10.0-windows" --nologo
if($LASTEXITCODE -ne 0){throw 'Dremma scene qualification build failed.'}
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Qualification/bin/Release/net10.0-windows/Rat.StrideMcp.Qualification.dll') -Destination (Join-Path $repository 'build/stride-mcp/adapter')
$owner=@{}
$failure=$null
$cleanupError=$null
$controlledFailureTriggered=$false
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
        if($phase -eq 'dremma-scene' -and $ExerciseCleanupFailure){$arguments+='--fail-after-save'}
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
        if($phase -eq 'dremma-scene') {
            if((Get-FileHash -LiteralPath $scene -Algorithm SHA256).Hash -ne $ownedEditedHash){throw 'Dirty editor save did not persist exactly the owned Dremma edit.'}
        }
    }
    Write-Host "Current-editor Dremma edit/save/fresh-reopen PASS: $output"
} catch {$failure=$_}
finally {
    Stop-OwnedMcpProcess $owner
    $temporary=$null
    try {
        $currentHash=(Get-FileHash -LiteralPath $scene -Algorithm SHA256).Hash
        $cleanupState='already-original'
        if($currentHash -ne $initialHash) {
            if(-not $ownedEditedHash -or $currentHash -ne $ownedEditedHash) {
                throw 'Dremma changed after the invocation-owned save; refusing to overwrite an intervening edit.'
            }
            $temporary=$scene+'.rat-owned-restore-'+[Guid]::NewGuid().ToString('N')
            Copy-Item -LiteralPath $backup -Destination $temporary
            Move-Item -LiteralPath $temporary -Destination $scene -Force
            if((Get-FileHash -LiteralPath $scene -Algorithm SHA256).Hash -ne $initialHash){throw 'Dremma cleanup restore hash mismatch.'}
            $cleanupState='restored-invocation-owned-edit'
        }
        [ordered]@{passed=$true;state=$cleanupState;initialSha256=$initialHash;ownedEditedSha256=$ownedEditedHash;finalSha256=(Get-FileHash -LiteralPath $scene -Algorithm SHA256).Hash} |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'cleanup.json') -Encoding UTF8
    } catch {
        $cleanupError=$_
        if($temporary -and (Test-Path -LiteralPath $temporary)){Remove-Item -LiteralPath $temporary}
        [ordered]@{passed=$false;error=$_.Exception.Message;initialSha256=$initialHash;ownedEditedSha256=$ownedEditedHash;currentSha256=$(if(Test-Path -LiteralPath $scene){(Get-FileHash -LiteralPath $scene -Algorithm SHA256).Hash})} |
            ConvertTo-Json | Set-Content -LiteralPath (Join-Path $output 'cleanup.json') -Encoding UTF8
    }
}
if($cleanupError){throw $cleanupError}
if($failure) {
    if($ExerciseCleanupFailure) {
        $controlledClient=Join-Path $output 'result.json.client.json'
        if(Test-Path -LiteralPath $controlledClient) {
            $controlledEvidence=Get-Content -LiteralPath $controlledClient -Raw | ConvertFrom-Json
            $controlledFailureTriggered=$controlledEvidence.controlledFailureAfterSave -eq $true
        }
    }
    if(-not $ExerciseCleanupFailure -or -not $controlledFailureTriggered){throw $failure}
    Write-Host "Controlled post-save failure cleanup PASS: $output"
}
