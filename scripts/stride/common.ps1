Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
$script:StrideLockFile = Join-Path $PSScriptRoot '../../tools/stride/engine.lock.json'
$script:StrideLock = Get-Content -LiteralPath $script:StrideLockFile -Raw | ConvertFrom-Json

function Get-StrideIntegrationCommit {
    if($script:StrideLock.PSObject.Properties['engineCommit']){return [string]$script:StrideLock.engineCommit}
    return [string]$script:StrideLock.upstreamCommit
}

function Invoke-StrideGit {
    param([string]$Root, [string[]]$Arguments)
    # Windows PowerShell wraps even successful git progress on stderr as ErrorRecord.
    $previousPreference = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        $output = & git -C $Root @Arguments 2>&1
        $gitExit = $LASTEXITCODE
    } finally { $ErrorActionPreference = $previousPreference }
    if ($gitExit -ne 0) { throw "git $($Arguments -join ' ') failed: $output" }
    return $output
}

function Get-StrideCheckoutPath {
    param([string]$Path)
    if (-not $Path) {
        $Path = Join-Path (Split-Path $script:StrideLockFile) $script:StrideLock.defaultCheckout
    }
    return $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($Path).TrimEnd('\', '/')
}

function Assert-StrideCheckout {
    param([string]$Root)
    if (-not (Test-Path -LiteralPath (Join-Path $Root '.git'))) {
        throw "Not a dedicated Stride checkout: $Root. Existing directories are never overwritten."
    }
    $top = [IO.Path]::GetFullPath([string](Invoke-StrideGit $Root @('rev-parse', '--show-toplevel'))).TrimEnd('\', '/')
    if ($top -ne $Root) { throw "Checkout must be the repository root: $top" }
    $remote = [string](Invoke-StrideGit $Root @('remote', 'get-url', $script:StrideLock.upstreamRemote))
    if ($remote -ne $script:StrideLock.upstreamUrl) { throw "Unexpected upstream URL: $remote" }
    $dirty = Invoke-StrideGit $Root @('status', '--porcelain', '--untracked-files=all')
    if ($dirty) { throw "Stride checkout has uncommitted files. Commit or preserve them separately before retrying: $Root" }
    $branch = [string](Invoke-StrideGit $Root @('branch', '--show-current'))
    if ($branch -ne $script:StrideLock.localBranch) { throw "Expected branch $($script:StrideLock.localBranch), found '$branch'. No checkout performed." }
    & git -C $Root merge-base --is-ancestor $script:StrideLock.upstreamCommit HEAD
    if ($LASTEXITCODE -ne 0) { throw 'HEAD does not descend from the locked upstream commit. No reset performed.' }
    return [string](Invoke-StrideGit $Root @('rev-parse', 'HEAD'))
}

function Assert-StrideLfs {
    param([string]$Root)
    Invoke-StrideGit $Root @('lfs', 'fsck') | Write-Host
    $pointers = @(Invoke-StrideGit $Root @('lfs', 'ls-files') | Where-Object { $_ -match '^[0-9a-f]+ - ' })
    if ($pointers.Count) { throw "Unmaterialized LFS files remain ($($pointers.Count)); run bootstrap with network access." }
}
