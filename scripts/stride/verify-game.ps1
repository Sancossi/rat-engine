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
        Write-Host "Running $Name"
        [IO.Directory]::CreateDirectory($evidence) | Out-Null
        # Native process quoting preserves spaces; these paths are generated locally and contain no quotes.
        $commandArguments = @('--smoke-frames', '360', '--evidence-dir', $evidence) + $Arguments
        $quotedArguments = @($commandArguments | ForEach-Object { '"' + $_ + '"' })
        $process = Start-Process -FilePath $exe -ArgumentList $quotedArguments -WorkingDirectory $unrelatedCwd -WindowStyle Hidden -PassThru
        # Stride throttles hidden/unfocused windows to 15 Hz: 1440 frames need 96 seconds.
        # Allow bounded startup/capture margin without disabling normal engine throttling.
        $timeoutMs = if (@('edges','body','layered','mixed','portals') | Where-Object { $Arguments -contains $_ }) { 180000 } else { 30000 }
        if (-not $process.WaitForExit($timeoutMs)) {
            Stop-Process -Id $process.Id -ErrorAction SilentlyContinue
            throw "$Name exceeded $($timeoutMs / 1000) seconds; only its spawned process was stopped."
        }
        $code = $process.ExitCode
        Write-Host "$Name exit $code; evidence $evidence"
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
    # Preserve the previous bounded-edge camera acceptance with authored blockers;
    # the default P1.3 map now intentionally permits falling and recovery.
    $edgeContent = Join-Path $root 'bounded-edge-content'
    Copy-Item -LiteralPath (Join-Path $app 'Content') -Destination $edgeContent -Recurse
    $edgeScene = Get-Content -LiteralPath (Join-Path $edgeContent 'courtyard.json') -Raw | ConvertFrom-Json
    $edgeScene.walls += @(
        @{id='edge-test-east';min=@{x=8;y=0;z=-6};max=@{x=8.5;y=2;z=6.5}},
        @{id='edge-test-south';min=@{x=-8;y=0;z=6};max=@{x=8.5;y=2;z=6.5}}
    )
    $edgeScene | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $edgeContent 'courtyard.json') -Encoding UTF8
    foreach ($size in @('4.5', '7')) {
        $name = "camera-edges-$size"
        $results += Invoke-GameScenario $name @('--camera-size', $size, '--smoke-route', 'edges', '--smoke-frames', '1440', '--content-dir', $edgeContent) $true
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
        if (-not $run.bodyComplete -or $run.width -ne $resolution[0] -or $run.height -ne $resolution[1] -or $run.ladderPausePassed -ne $true) { throw "$name did not complete the real body route and ladder pause." }
        $pauseStart = @($run.sessionMilestones | Where-Object name -eq 'ladder-pause-start')
        $pauseEnd = @($run.sessionMilestones | Where-Object name -eq 'ladder-paused')
        if ($pauseStart.Count -eq 1 -and $pauseEnd.Count -eq 1 -and $pauseEnd[0].frame-$pauseStart[0].frame -ne 12) { throw "$name did not cross a complete animation frame interval while paused." }
        if ($pauseStart.Count -ne 1 -or $pauseEnd.Count -ne 1 -or $pauseStart[0].session.Mode -ne 'Paused' -or $pauseEnd[0].session.Mode -ne 'Paused' -or $pauseStart[0].session.Ticks -ne $pauseEnd[0].session.Ticks -or $pauseStart[0].leaderAnimationFrame -ne $pauseEnd[0].leaderAnimationFrame) { throw "$name advanced simulation or sprite animation while paused." }
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
    $largeScene.spawns = @($largeScene.spawns | Where-Object id -eq 'entry')
    $largeScene.spawns[0].position.y = 4194304
    $largeScene.ramps = @(); $largeScene.portals = @(); $largeScene.decorations = @(); $largeScene.occlusionGroups = @()
    $largeScene.walls = @()
    $largeScene.structures = @($largeScene.structures | Where-Object id -eq 'upper-platform')
    $largeScene.structures[0].min.y = 4194305
    $largeScene.structures[0].max.y = 4194306
    foreach ($point in @('bottom', 'bottomEntry', 'bottomExit')) { $largeScene.ladders[0].$point.y = 4194304 }
    foreach ($point in @('top', 'topEntry', 'topExit')) { $largeScene.ladders[0].$point.y = 4194306 }
    $largeScene | ConvertTo-Json -Depth 8 | Set-Content -LiteralPath (Join-Path $largeContent 'courtyard.json') -Encoding UTF8
    @{schemaVersion=1;startScene=$largeScene.id;scenes=@(@{id=$largeScene.id;path='courtyard.json'})} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $largeContent 'project.json') -Encoding UTF8
    $results += Invoke-GameScenario 'large-y-ladder-startup' @('--content-dir', $largeContent, '--smoke-route', 'zoom', '--smoke-frames', '60') $true
    if (-not (Test-Path -LiteralPath (Join-Path $root 'large-y-ladder-startup/frame-0030.png'))) { throw 'Large-Y ladder startup did not render a frame.' }

    foreach ($failure in @('missing-png', 'corrupt-png', 'missing-json', 'malformed-json', 'missing-coordinate-json', 'missing-font', 'corrupt-font', 'blocked-ladder-exit', 'missing-ladder-coordinate', 'missing-ramp-coordinate', 'missing-project', 'bad-portal-target', 'bad-occlusion-parent')) {
        $content = Join-Path $root "$failure-content"
        Copy-Item -LiteralPath (Join-Path $app 'Content') -Destination $content -Recurse
        switch ($failure) {
            'missing-png' { Rename-Item -LiteralPath (Join-Path $content 'sprites/rat.png') -NewName 'rat.png.absent' }
            'corrupt-png' { [IO.File]::WriteAllBytes((Join-Path $content 'sprites/rat.png'), [byte[]](1,2,3,4,5)) }
            'missing-json' { Rename-Item -LiteralPath (Join-Path $content 'courtyard.json') -NewName 'courtyard.json.absent' }
            'malformed-json' { [IO.File]::WriteAllText((Join-Path $content 'courtyard.json'), '{ malformed JSON') }
            'missing-coordinate-json' {
                $document = Get-Content -LiteralPath (Join-Path $content 'courtyard.json') -Raw | ConvertFrom-Json
                $document.spawns[0].position.PSObject.Properties.Remove('x')
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
            'missing-ramp-coordinate' {
                $document = Get-Content -LiteralPath (Join-Path $content 'courtyard.json') -Raw | ConvertFrom-Json
                $document.ramps[0].min.PSObject.Properties.Remove('x')
                $document | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $content 'courtyard.json') -Encoding UTF8
            }
            'missing-project' { Rename-Item -LiteralPath (Join-Path $content 'project.json') -NewName 'project.json.absent' }
            'bad-portal-target' {
                $document = Get-Content -LiteralPath (Join-Path $content 'courtyard.json') -Raw | ConvertFrom-Json
                $document.portals[0].targetSpawn = 'missing-spawn'
                $document | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $content 'courtyard.json') -Encoding UTF8
            }
            'bad-occlusion-parent' {
                $document = Get-Content -LiteralPath (Join-Path $content 'courtyard.json') -Raw | ConvertFrom-Json
                ($document.occlusionGroups | Where-Object id -eq 'bridge-rail-cut').hideWith = 'missing-parent'
                $document | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $content 'courtyard.json') -Encoding UTF8
            }
        }
        $results += Invoke-GameScenario $failure @('--content-dir', $content) $false
    }
    foreach ($resolution in @(@(1280,720), @(1920,1080))) {
        $name = "layered-$($resolution[0])x$($resolution[1])"
        $results += Invoke-GameScenario $name @('--width',[string]$resolution[0],'--height',[string]$resolution[1],'--smoke-route','layered','--smoke-frames','1800') $true
        $run = Get-Content -LiteralPath (Join-Path $root "$name/run.json") -Raw | ConvertFrom-Json
        if (-not $run.sessionComplete -or $run.width -ne $resolution[0] -or $run.height -ne $resolution[1]) { throw "$name incomplete layered route." }
        foreach ($milestone in @('arch-empty','ramp-entry','ramp-ascent','ramp-crest','bridge-upper','upper-rail','ramp-descent','bridge-lower','lower-forward','cut-restored','lower-reverse','offcentre-behind-wall')) {
            $item = @($run.sessionMilestones | Where-Object name -eq $milestone)
            if ($item.Count -ne 1 -or -not (Test-Path -LiteralPath (Join-Path $root "$name/session-$milestone.png"))) { throw "$name missing $milestone." }
            $item = $item[0]
            if ($milestone -in @('bridge-upper','upper-rail') -and ($item.session.Leader.Position.Y -ne 1.6 -or $item.hidden -contains 'bridge-cut')) { throw "$name lost protected upper deck." }
            if ($milestone -in @('bridge-lower','lower-reverse','offcentre-behind-wall') -and ($item.session.Leader.Position.Y -ne 0 -or $item.hidden -notcontains 'bridge-cut' -or $item.hidden -notcontains 'bridge-rail-cut')) { throw "$name failed local lower cut." }
            if ($milestone -eq 'upper-rail' -and $item.hidden -notcontains 'bridge-rail-cut') { throw "$name rail did not hide independently." }
            if ($milestone -eq 'arch-empty' -and $item.hidden -contains 'arch-cut') { throw "$name hid empty arch." }
            if ($milestone -eq 'cut-restored' -and $item.hidden -contains 'bridge-cut') { throw "$name failed cut restoration." }
            if ($item.hidden -contains 'north-independent') { throw "$name hid unrelated wall beyond leader depth." }
        }
        $upper = ($run.sessionMilestones | Where-Object name -eq 'bridge-upper').session.Leader.Position
        $lower = ($run.sessionMilestones | Where-Object name -eq 'bridge-lower').session.Leader.Position
        if ([Math]::Abs($upper.X-$lower.X) -gt .04 -or [Math]::Abs($upper.Z-$lower.Z) -gt .04) { throw "$name did not demonstrate identical XZ on two levels." }
    }
    $results += Invoke-GameScenario 'mixed-companions' @('--smoke-route','mixed','--smoke-frames','1500') $true
    $run = Get-Content -LiteralPath (Join-Path $root 'mixed-companions/run.json') -Raw | ConvertFrom-Json
    $mixed = @($run.sessionMilestones | Where-Object name -eq 'mixed-companions')
    if (-not $run.sessionComplete -or $mixed.Count -ne 1 -or $mixed[0].actorVisible[1] -ne $true -or $mixed[0].actorVisible[2] -ne $false -or $mixed[0].companions[0].Position.Y -ne 0 -or $mixed[0].companions[1].Position.Y -ne 1.6) { throw 'Mixed-height companion locality not demonstrated.' }
    $results += Invoke-GameScenario 'portal-roundtrips' @('--smoke-route','portals','--smoke-frames','1800') $true
    $run = Get-Content -LiteralPath (Join-Path $root 'portal-roundtrips/run.json') -Raw | ConvertFrom-Json
    if (-not $run.sessionComplete -or $run.portalLegs -ne 20 -or $run.session.WorldRevision -ne 20) { throw 'Did not complete ten round trips.' }
    foreach ($item in $run.sessionMilestones) {
        if ($item.hidden.Count -ne 0 -or $item.companions[0].Position.Y -ne $item.session.Leader.Position.Y) { throw 'Portal did not reset party/occlusion.' }
        $sameScene = @($run.sessionMilestones | Where-Object { $_.session.Leader.SceneId -eq $item.session.Leader.SceneId })
        # Stable active-bundle size plus explicit code ownership review; this is
        # not a GPU allocator/memory profiler and does not itself prove no leaks.
        if (@($sameScene.ownedBuffers | Select-Object -Unique).Count -ne 1) { throw 'Active scene buffer count changed across transitions.' }
    }
    $results += Invoke-GameScenario 'renderer-candidate-failure' @('--smoke-route','portal-failure') $true
    $run = Get-Content -LiteralPath (Join-Path $root 'renderer-candidate-failure/run.json') -Raw | ConvertFrom-Json
    if (-not $run.sessionComplete -or $run.session.WorldRevision -ne 0 -or $run.scene -ne 'expedition_courtyard' -or $run.session.Hint -notmatch 'Diagnostic renderer candidate rejected') { throw 'Renderer candidate failure replaced old scene or lost diagnostic.' }
    foreach ($fixture in @('courtyard','sluice','upper-void')) {
        $content = Join-Path $root "recovery-$fixture-content"
        Copy-Item -LiteralPath (Join-Path $app 'Content') -Destination $content -Recurse
        $sceneFile = if ($fixture -eq 'sluice') { 'sluice.json' } else { 'courtyard.json' }
        $document = Get-Content -LiteralPath (Join-Path $content $sceneFile) -Raw | ConvertFrom-Json
        $document.portals = @(); $document.spawns = @($document.spawns | Where-Object id -eq 'entry')
        if ($fixture -eq 'upper-void') {
            $document.floor.min.x=-4; $document.floor.max.x=0
            $document.walls=@(); $document.ladders=@(); $document.ramps=@(); $document.decorations=@(); $document.occlusionGroups=@()
            $document.structures=@(@{id='upper-void-deck';min=@{x=1;y=1.4;z=1};max=@{x=4;y=1.6;z=3}})
            $document.spawns[0].position=@{x=2.5;y=1.6;z=2}
        }
        $document | ConvertTo-Json -Depth 12 | Set-Content -LiteralPath (Join-Path $content $sceneFile) -Encoding UTF8
        @{schemaVersion=1;startScene=$document.id;scenes=@(@{id=$document.id;path=$sceneFile})} | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $content 'project.json') -Encoding UTF8
        $name = "recovery-$fixture"
        $results += Invoke-GameScenario $name @('--content-dir',$content,'--smoke-route','recovery') $true
        $run = Get-Content -LiteralPath (Join-Path $root "$name/run.json") -Raw | ConvertFrom-Json
        $expectedY = if ($fixture -eq 'upper-void') {1.6} else {0}
        if (-not $run.sessionComplete -or $run.session.WorldRevision -ne 1 -or $run.finalPosition.y -ne $expectedY -or $run.finalSnapshot.State -ne 'Standing' -or $run.hiddenGroups.Count -ne 0) { throw "$name did not restore correct safe layer." }
        foreach ($pose in $run.companions) { if ($pose.Position.Y -ne $expectedY -or [Math]::Abs($pose.Position.X-$run.finalPosition.x) -gt .00001) { throw "$name retained old party history." } }
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
