[CmdletBinding()]
param([Parameter(Mandatory = $true)][string]$PackageZip)

Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
try {
    $zip = (Resolve-Path -LiteralPath $PackageZip).Path
    $repo = [IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
    $root = Join-Path (Split-Path $repo) ('rat-expedition-validation/' + (Get-Date -Format 'yyyyMMdd-HHmmss-fff'))
    [IO.Directory]::CreateDirectory($root) | Out-Null
    Add-Type -AssemblyName System.IO.Compression.FileSystem
    $app = Join-Path $root 'extracted'
    [IO.Compression.ZipFile]::ExtractToDirectory($zip, $app)
    $exe = Join-Path $app 'Rat.Expedition.Windows.exe'
    $unrelatedCwd = Join-Path $root 'unrelated-working-directory'
    [IO.Directory]::CreateDirectory($unrelatedCwd) | Out-Null
    $results = @()

    function Invoke-GameScenario {
        param([string]$Name, [string[]]$Arguments, [bool]$ExpectSuccess)
        $evidence = Join-Path $root $Name
        [IO.Directory]::CreateDirectory($evidence) | Out-Null
        # Native process quoting preserves spaces; these paths are generated locally and contain no quotes.
        $commandArguments = @('--smoke-frames', '360', '--evidence-dir', $evidence) + $Arguments
        $quotedArguments = @($commandArguments | ForEach-Object { '"' + $_ + '"' })
        $process = Start-Process -FilePath $exe -ArgumentList $quotedArguments -WorkingDirectory $unrelatedCwd -WindowStyle Hidden -PassThru
        # Stride throttles hidden/unfocused windows to 15 Hz: 1440 frames need 96 seconds.
        # Allow bounded startup/capture margin without disabling normal engine throttling.
        $timeoutMs = if ($Arguments -contains 'edges' -or $Arguments -contains 'body') { 150000 } else { 30000 }
        if (-not $process.WaitForExit($timeoutMs)) {
            Stop-Process -Id $process.Id -ErrorAction SilentlyContinue
            throw "$Name exceeded $($timeoutMs / 1000) seconds; only its spawned process was stopped."
        }
        $code = $process.ExitCode
        if (($ExpectSuccess -and $code -ne 0) -or (-not $ExpectSuccess -and $code -eq 0)) {
            throw "$Name unexpected exit $code. Inspect $evidence"
        }
        if (-not $ExpectSuccess -and -not (Test-Path -LiteralPath (Join-Path $evidence 'error.log'))) {
            throw "$Name failed without a readable error log."
        }
        return [ordered]@{ scenario = $Name; exitCode = $code; evidence = $evidence }
    }

    foreach ($resolution in @(@(1280,720), @(1920,1080))) {
        $name = "gpu-$($resolution[0])x$($resolution[1])"
        $results += Invoke-GameScenario $name @('--width', [string]$resolution[0], '--height', [string]$resolution[1]) $true
        $run = Get-Content -LiteralPath (Join-Path $root "$name/run.json") -Raw | ConvertFrom-Json
        if ($run.width -ne $resolution[0] -or $run.height -ne $resolution[1] -or -not $run.adapter -or $run.finalPosition.z -ge 0 -or $run.focusProbePassed -ne $true) {
            throw "$name missing expected GPU dimensions/adapter/behind-wall route evidence."
        }
        foreach ($frame in @('frame-0030.png', 'frame-0180.png', 'frame-0360.png')) {
            if (-not (Test-Path -LiteralPath (Join-Path $root "$name/$frame"))) { throw "$name missing $frame" }
        }
        foreach ($sample in $run.cameraSamples) {
            if ($sample.quadLeft -lt 0 -or $sample.quadRight -gt 1 -or $sample.quadTop -lt 0 -or $sample.quadBottom -gt 1 -or $sample.spriteHeightFraction -lt .12) {
                throw "$name hero framing is outside the screen or too small."
            }
        }
    }
    foreach ($size in @('4.5', '7')) {
        $name = "camera-edges-$size"
        $results += Invoke-GameScenario $name @('--camera-size', $size, '--smoke-route', 'edges', '--smoke-frames', '1440') $true
        $run = Get-Content -LiteralPath (Join-Path $root "$name/run.json") -Raw | ConvertFrom-Json
        if ($run.camera.size -ne [double]::Parse($size, [Globalization.CultureInfo]::InvariantCulture) -or $run.cameraSamples.Count -lt 40) { throw "$name missing camera evidence." }
        foreach ($sample in $run.cameraSamples) {
            if ($sample.quadLeft -lt 0 -or $sample.quadRight -gt 1 -or $sample.quadTop -lt 0 -or $sample.quadBottom -gt 1) { throw "$name hero left screen at frame $($sample.frame)." }
        }
        $x = $run.cameraSamples | Measure-Object worldX -Minimum -Maximum
        $z = $run.cameraSamples | Measure-Object worldZ -Minimum -Maximum
        if ($x.Minimum -gt -7.2 -or $x.Maximum -lt 7.7 -or $z.Minimum -gt -5.2 -or $z.Maximum -lt 5.7) { throw "$name did not reach all map edges." }
    }
    $results += Invoke-GameScenario 'camera-wheel' @('--smoke-route', 'zoom') $true
    $wheelRun = Get-Content -LiteralPath (Join-Path $root 'camera-wheel/run.json') -Raw | ConvertFrom-Json
    foreach ($expected in @(@(60,4.5), @(120,7), @(150,7), @(180,4.5), @(240,7), @(300,5))) {
        $sample = @($wheelRun.cameraSamples | Where-Object frame -eq $expected[0])
        if ($sample.Count -ne 1 -or $sample[0].size -ne $expected[1]) { throw "Wheel clamp/discard failed at frame $($expected[0])." }
    }
    if ($wheelRun.focusZoomProbePassed -ne $true) { throw 'Focus regain accepted a stale wheel delta.' }
    foreach ($resolution in @(@(1280,720), @(1920,1080))) {
        $name = "body-$($resolution[0])x$($resolution[1])"
        $results += Invoke-GameScenario $name @('--width', [string]$resolution[0], '--height', [string]$resolution[1], '--smoke-route', 'body', '--smoke-frames', '1200') $true
        $run = Get-Content -LiteralPath (Join-Path $root "$name/run.json") -Raw | ConvertFrom-Json
        if (-not $run.bodyComplete -or $run.width -ne $resolution[0] -or $run.height -ne $resolution[1]) { throw "$name did not complete the real body route." }
        foreach ($milestone in @('standing-blocked','crouched','blocked-stand','clear-standing','climb-up','upper-exit','held-interact-top','top-walking','climb-down','lower-exit')) {
            $item = @($run.bodyMilestones | Where-Object name -eq $milestone)
            if ($item.Count -ne 1 -or -not (Test-Path -LiteralPath (Join-Path $root "$name/body-$milestone.png"))) { throw "$name missing $milestone evidence." }
            $item = $item[0]
            $state = if ($milestone -in @('crouched','blocked-stand')) {'Crouched'} elseif ($milestone -in @('climb-up','climb-down')) {'Climbing'} else {'Standing'}
            if ($item.state -ne $state) { throw "$name unexpected FSM state for $milestone." }
            if ($state -eq 'Crouched' -and $item.height -ne .4) { throw "$name crouched body height mismatch." }
            if ($state -ne 'Crouched' -and $item.height -ne .8) { throw "$name standing body height mismatch." }
            if ($milestone -eq 'blocked-stand' -and (-not $item.StandBlocked -or $item.Hint -ne 'Здесь нельзя встать')) { throw "$name missing Cyrillic blocked-stand hint." }
            if ($milestone -in @('upper-exit','held-interact-top','top-walking') -and $item.y -ne 1.6) { throw "$name lost upper support." }
            if ($milestone -eq 'lower-exit' -and $item.y -ne 0) { throw "$name wrong lower exit." }
        }
    }
    # Accepted finite coordinates where float ULP exceeds rung spacing used to hang
    # scene construction. Verify the actual renderer terminates, without promising
    # large-world traversal or sub-unit visual precision at this coordinate scale.
    $largeContent = Join-Path $root 'large-y-ladder-content'
    Copy-Item -LiteralPath (Join-Path $app 'Content') -Destination $largeContent -Recurse
    $largeScene = Get-Content -LiteralPath (Join-Path $largeContent 'courtyard.json') -Raw | ConvertFrom-Json
    $largeScene.floor.min.y = 4194303
    $largeScene.floor.max.y = 4194304
    $largeScene.spawn.y = 4194304
    $largeScene.walls = @()
    $largeScene.structures = @($largeScene.structures | Where-Object id -eq 'upper-platform')
    $largeScene.structures[0].min.y = 4194305
    $largeScene.structures[0].max.y = 4194306
    foreach ($point in @('bottom', 'bottomEntry', 'bottomExit')) { $largeScene.ladders[0].$point.y = 4194304 }
    foreach ($point in @('top', 'topEntry', 'topExit')) { $largeScene.ladders[0].$point.y = 4194306 }
    $largeScene | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $largeContent 'courtyard.json') -Encoding UTF8
    $results += Invoke-GameScenario 'large-y-ladder-startup' @('--content-dir', $largeContent, '--smoke-route', 'zoom', '--smoke-frames', '60') $true
    if (-not (Test-Path -LiteralPath (Join-Path $root 'large-y-ladder-startup/frame-0030.png'))) { throw 'Large-Y ladder startup did not render a frame.' }

    foreach ($failure in @('missing-png', 'corrupt-png', 'missing-json', 'malformed-json', 'missing-coordinate-json', 'missing-font', 'corrupt-font', 'blocked-ladder-exit', 'missing-ladder-coordinate')) {
        $content = Join-Path $root "$failure-content"
        Copy-Item -LiteralPath (Join-Path $app 'Content') -Destination $content -Recurse
        switch ($failure) {
            'missing-png' { Rename-Item -LiteralPath (Join-Path $content 'sprites/rat.png') -NewName 'rat.png.absent' }
            'corrupt-png' { [IO.File]::WriteAllBytes((Join-Path $content 'sprites/rat.png'), [byte[]](1,2,3,4,5)) }
            'missing-json' { Rename-Item -LiteralPath (Join-Path $content 'courtyard.json') -NewName 'courtyard.json.absent' }
            'malformed-json' { [IO.File]::WriteAllText((Join-Path $content 'courtyard.json'), '{ malformed JSON') }
            'missing-coordinate-json' {
                $document = Get-Content -LiteralPath (Join-Path $content 'courtyard.json') -Raw | ConvertFrom-Json
                $document.spawn.PSObject.Properties.Remove('x')
                $document | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $content 'courtyard.json') -Encoding UTF8
            }
            'missing-font' { Rename-Item -LiteralPath (Join-Path $content 'fonts/NotoSans-Regular.ttf') -NewName 'NotoSans-Regular.ttf.absent' }
            'corrupt-font' { [IO.File]::WriteAllBytes((Join-Path $content 'fonts/NotoSans-Regular.ttf'), [byte[]](1,2,3,4,5)) }
            'blocked-ladder-exit' {
                $document = Get-Content -LiteralPath (Join-Path $content 'courtyard.json') -Raw | ConvertFrom-Json
                $document.ladders[0].topExit.x = 5
                $document | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $content 'courtyard.json') -Encoding UTF8
            }
            'missing-ladder-coordinate' {
                $document = Get-Content -LiteralPath (Join-Path $content 'courtyard.json') -Raw | ConvertFrom-Json
                $document.ladders[0].top.PSObject.Properties.Remove('y')
                $document | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $content 'courtyard.json') -Encoding UTF8
            }
        }
        $results += Invoke-GameScenario $failure @('--content-dir', $content) $false
    }
    [ordered]@{ packageZip = $zip; packageSha256 = (Get-FileHash -LiteralPath $zip -Algorithm SHA256).Hash
        extractedApplication = $app; workingDirectory = $unrelatedCwd; scenarios = $results
        limitation = 'Actual GPU and isolated package on this development PC; not a clean-machine or manual keyboard playtest.' } |
        ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $root 'verification.json') -Encoding UTF8
    Write-Host "Game verification passed: $root/verification.json"
} catch {
    Write-Error $_ -ErrorAction Continue
    exit 1
}
