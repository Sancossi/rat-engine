[CmdletBinding()]
param([string]$CheckoutPath)

. (Join-Path $PSScriptRoot 'common.ps1')
try {
    Get-Command git -ErrorAction Stop | Out-Null
    & git lfs version
    if ($LASTEXITCODE -ne 0) { throw 'Git LFS is required.' }
    $root = Get-StrideCheckoutPath $CheckoutPath
    if (Test-Path -LiteralPath $root) {
        $head = Assert-StrideCheckout $root
        Write-Host "Preserving existing branch and HEAD $head"
    } else {
        # No checkout until the locked object is present; do not follow a moving branch tip.
        & git clone --origin $script:StrideLock.upstreamRemote --no-checkout $script:StrideLock.upstreamUrl $root
        if ($LASTEXITCODE -ne 0) { throw "Clone failed. Partial directory preserved at $root; inspect it before retrying." }
        Invoke-StrideGit $root @('cat-file', '-e', "$($script:StrideLock.upstreamCommit)^{commit}") | Out-Null
        Invoke-StrideGit $root @('lfs', 'install', '--local') | Write-Host
        Invoke-StrideGit $root @('switch', '-c', $script:StrideLock.localBranch, $script:StrideLock.upstreamCommit) | Write-Host
        $head = Assert-StrideCheckout $root
    }
    Invoke-StrideGit $root @('lfs', 'pull', $script:StrideLock.upstreamRemote) | Write-Host
    Assert-StrideLfs $root
    Write-Host "Stride ready: $root ($head)"
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
