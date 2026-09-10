[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][string]$Executable,
    [Parameter(Mandatory=$true)][string]$EvidenceDirectory,
    [Parameter(Mandatory=$true)][string]$WorkingDirectory
)
Set-StrictMode -Version Latest
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
if (-not ('RatSmokeWindow' -as [type])) {
    Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public static class RatSmokeWindow {
    [DllImport("user32.dll")] public static extern bool ShowWindowAsync(IntPtr h, int command);
    [DllImport("user32.dll")] public static extern bool SetForegroundWindow(IntPtr h);
    [DllImport("user32.dll")] public static extern IntPtr GetForegroundWindow();
    [DllImport("user32.dll")] public static extern bool IsIconic(IntPtr h);
    [DllImport("user32.dll")] public static extern uint GetWindowThreadProcessId(IntPtr h, out uint processId);
}
'@
}
[IO.Directory]::CreateDirectory($EvidenceDirectory) | Out-Null
$results = @()
foreach ($state in @('focused','unfocused','hidden','minimized')) {
    $directory = Join-Path $EvidenceDirectory $state
    if (Test-Path -LiteralPath $directory) { throw "Refusing to reuse state evidence: $directory" }
    [IO.Directory]::CreateDirectory($directory) | Out-Null
    $arguments = @('--smoke-route','window-states','--smoke-frames','240','--evidence-dir',$directory)
    $quoted = @($arguments | ForEach-Object { '"' + $_ + '"' })
    $start = @{FilePath=$Executable;ArgumentList=$quoted;WorkingDirectory=$WorkingDirectory;PassThru=$true}
    if ($state -ne 'focused') { $start.WindowStyle = 'Hidden' }
    $process = Start-Process @start
    try {
        $deadline = [DateTime]::UtcNow.AddSeconds(30)
        $progressPath = Join-Path $directory 'smoke-progress.json'
        $progress = $null
        while (-not $progress -and -not $process.HasExited -and [DateTime]::UtcNow -lt $deadline) {
            if (Test-Path -LiteralPath $progressPath) {
                try { $progress = Get-Content -LiteralPath $progressPath -Raw | ConvertFrom-Json } catch { }
            }
            if (-not $progress) { Start-Sleep -Milliseconds 25 }
        }
        if (-not $progress) { throw "$state did not produce startup/frame progress." }
        $handle = [IntPtr][long]$progress.hwnd
        [uint32]$windowProcessId = 0
        [void][RatSmokeWindow]::GetWindowThreadProcessId($handle, [ref]$windowProcessId)
        if ($windowProcessId -ne $process.Id) { throw 'Observed root HWND does not belong to our spawned game.' }
        if ($state -eq 'focused') {
            $showResult = [RatSmokeWindow]::ShowWindowAsync($handle, 9)
            $setForegroundResult = [RatSmokeWindow]::SetForegroundWindow($handle)
            $activationSamples = @()
            $activationDeadline = [DateTime]::UtcNow.AddSeconds(2)
            do {
                $foregroundHandle = [RatSmokeWindow]::GetForegroundWindow()
                [uint32]$foregroundProcessId = 0
                [void][RatSmokeWindow]::GetWindowThreadProcessId($foregroundHandle, [ref]$foregroundProcessId)
                $activationSamples += [ordered]@{utc=[DateTime]::UtcNow.ToString('O');hwnd=$foregroundHandle.ToInt64();processId=$foregroundProcessId;isOwned=$foregroundHandle -eq $handle}
                if ($foregroundHandle -eq $handle) { break }
                Start-Sleep -Milliseconds 25
            } while ([DateTime]::UtcNow -lt $activationDeadline)
            [ordered]@{showWindowAsync=$showResult;setForegroundWindow=$setForegroundResult;ownedHwnd=$handle.ToInt64();ownedProcessId=$process.Id;samples=$activationSamples} |
                ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $directory 'activation.json') -Encoding UTF8
            if ($foregroundHandle -ne $handle) { throw 'Own visible window did not become the foreground window within two seconds.' }
        } elseif ($state -eq 'hidden') {
            [RatSmokeWindow]::ShowWindowAsync($handle, 0) | Out-Null
        } else {
            # Minimizing our own window relinquishes focus without targeting another app.
            [RatSmokeWindow]::ShowWindowAsync($handle, 6) | Out-Null
            if ($state -eq 'unfocused') {
                $minimizeDeadline = [DateTime]::UtcNow.AddSeconds(2)
                while (-not [RatSmokeWindow]::IsIconic($handle) -and [DateTime]::UtcNow -lt $minimizeDeadline) { Start-Sleep -Milliseconds 10 }
                if (-not [RatSmokeWindow]::IsIconic($handle)) { throw 'Own window did not minimize before unfocused restore.' }
                [RatSmokeWindow]::ShowWindowAsync($handle, 4) | Out-Null # show, no activation
            }
        }
        if (-not $process.WaitForExit(30000)) { throw "$state smoke exceeded 30 seconds after startup." }
        if ($process.ExitCode -ne 0) { throw "$state smoke failed: $($process.ExitCode)." }
        $run = Get-Content -LiteralPath (Join-Path $directory 'run.json') -Raw | ConvertFrom-Json
        $samples = @($run.smokeTiming.samples | Where-Object { $_.frame -ge 60 })
        if ($samples.Count -lt 6 -or $run.frames -ne 240 -or $run.focusProbePassed -ne $true -or
            $run.scene -ne 'expedition_dremma' -or $run.nativeVisuals -ne 14 -or $run.nativeMeshes -ne 97 -or
            $run.nativeMaterialSlots -ne 79 -or $run.activeNativeLeases -ne 1 -or $run.camera.size -ne 11.25) {
            throw "$state missing production Dremma progress, native resources, or deliberate pause probe."
        }
        $shutdown = Get-Content -LiteralPath (Join-Path $directory 'shutdown.json') -Raw | ConvertFrom-Json
        if ($shutdown.activeNativeLeases -ne 0) { throw "$state retained native content after game shutdown." }
        foreach ($sample in $samples) {
            $matches = switch ($state) {
                'focused' { $sample.foreground -and $sample.visible -and -not $sample.isIconic }
                'unfocused' { -not $sample.foreground -and $sample.visible -and -not $sample.isIconic }
                'hidden' { -not $sample.visible -and -not $sample.foreground }
                'minimized' { $sample.isIconic -and $sample.windowMinimized -and -not $sample.foreground }
            }
            if (-not $matches) { throw "$state OS state was not maintained at frame $($sample.frame)." }
        }
        $first = $samples[0]; $last = $samples[-1]
        $frameDelta = $last.frame - $first.frame
        $hz = $frameDelta / ($last.seconds - $first.seconds)
        if ($hz -lt 35 -or $hz -gt 80 -or $last.draws-$first.draws -ne $frameDelta -or $last.ticks -le $first.ticks) {
            throw "$state rate/draw progression failed: $hz Hz. Inspect GPU load and samples."
        }
        $hashes = @()
        foreach ($frame in @(60,180)) {
            $path = Join-Path $directory ('frame-{0:D4}.png' -f $frame)
            $bitmap = [Drawing.Bitmap]::new($path)
            try {
                $colors = [Collections.Generic.HashSet[int]]::new()
                for ($y=0; $y -lt $bitmap.Height; $y+=16) { for ($x=0; $x -lt $bitmap.Width; $x+=16) { [void]$colors.Add($bitmap.GetPixel($x,$y).ToArgb()) } }
                if ($bitmap.Width -ne 1280 -or $bitmap.Height -ne 720 -or $colors.Count -lt 8) { throw "$state has an empty/invalid GPU frame." }
            } finally { $bitmap.Dispose() }
            $hashes += (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash
        }
        if ($hashes[0] -eq $hashes[1]) { throw "$state captures did not change during the moving route." }
        $results += [ordered]@{ scenario="window-$state"; exitCode=$process.ExitCode; evidence=$directory; steadyHz=$hz; firstFrame=$first.frame; lastFrame=$last.frame; ticks=$last.ticks-$first.ticks; finalTicks=$last.ticks; finalPosition=$run.finalPosition }
        Write-Host "window-$state passed: $([Math]::Round($hz,2)) Hz; $directory"
    } finally {
        if (-not $process.HasExited) { Stop-Process -Id $process.Id -ErrorAction SilentlyContinue }
        $process.Dispose()
    }
}
$rates = @($results.steadyHz | Measure-Object -Minimum -Maximum)
if ($rates[0].Maximum / $rates[0].Minimum -gt 1.5) { throw 'Window states have materially different steady rates.' }
foreach ($result in $results) {
    if ($result.finalTicks -ne $results[0].finalTicks -or
        [Math]::Abs($result.finalPosition.x-$results[0].finalPosition.x) -gt .0001 -or
        [Math]::Abs($result.finalPosition.y-$results[0].finalPosition.y) -gt .0001 -or
        [Math]::Abs($result.finalPosition.z-$results[0].finalPosition.z) -gt .0001) { throw 'Window state changed the deterministic route/tick result.' }
}
$results | ConvertTo-Json -Depth 6 | Set-Content -LiteralPath (Join-Path $EvidenceDirectory 'verification.json') -Encoding UTF8
$results
