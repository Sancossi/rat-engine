[CmdletBinding()]
param([Parameter(Mandatory=$true)][string]$VerifiedCache)
$ErrorActionPreference='Stop'
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$game=Join-Path $repository 'games/rat-expedition'
$destination=Join-Path $game '.packages'
$source=$ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($VerifiedCache)
$packages=@{}
foreach($name in @('Rat.Expedition.Authoring','Rat.Expedition.Authoring.Windows')){
    $lock=Get-Content -LiteralPath (Join-Path $game "$name/packages.lock.json") -Raw | ConvertFrom-Json
    foreach($framework in $lock.dependencies.PSObject.Properties){
        foreach($entry in $framework.Value.PSObject.Properties){
            if($entry.Name -like 'Stride.*' -and $entry.Name -notlike 'Stride.Dependencies.*'){
                $key=$entry.Name.ToLowerInvariant()+'/'+$entry.Value.resolved
                if($packages.ContainsKey($key) -and $packages[$key] -ne $entry.Value.contentHash){throw "Conflicting committed package hashes: $key"}
                $packages[$key]=$entry.Value.contentHash
            }
        }
    }
}
function Get-PackageHash([string]$File){
    $stream=[IO.File]::OpenRead($File)
    $sha=[Security.Cryptography.SHA512]::Create()
    try{return [Convert]::ToBase64String($sha.ComputeHash($stream))}finally{$stream.Dispose();$sha.Dispose()}
}
# Validate ALL source packages before changing the isolated destination cache.
foreach($key in $packages.Keys){
    $bits=$key.Split('/');$archive=Join-Path $source "$key/$($bits[0]).$($bits[1]).nupkg"
    if((Get-PackageHash $archive) -ne $packages[$key]){throw "Source archive does not match committed lock: $key"}
}
$quarantine=Join-Path $repository ('build/mcp/cache-cohort-before-'+(Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
foreach($key in $packages.Keys){
    $bits=$key.Split('/');$target=[IO.Path]::GetFullPath((Join-Path $destination $key))
    if(-not $target.StartsWith(([IO.Path]::GetFullPath($destination)+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase)){throw 'Cache target escapes game cache.'}
    $archive=Join-Path $target "$($bits[0]).$($bits[1]).nupkg"
    if((Test-Path -LiteralPath $archive) -and (Get-PackageHash $archive) -eq $packages[$key]){continue}
    if(Test-Path -LiteralPath $target){
        $backup=[IO.Path]::GetFullPath((Join-Path $quarantine $key))
        if(-not $backup.StartsWith(([IO.Path]::GetFullPath($quarantine)+[IO.Path]::DirectorySeparatorChar),[StringComparison]::OrdinalIgnoreCase)){throw 'Backup target escapes quarantine.'}
        New-Item -ItemType Directory -Force -Path (Split-Path $backup) | Out-Null
        Move-Item -LiteralPath $target -Destination $backup
    }
    New-Item -ItemType Directory -Force -Path (Split-Path $target) | Out-Null
    Copy-Item -LiteralPath (Join-Path $source $key) -Destination $target -Recurse
}
Write-Host "Verified $($packages.Count) pinned package archives against committed locks; isolated cache ready."
