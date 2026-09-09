[CmdletBinding()]
param(
    [Parameter(Mandatory=$true)][int]$EditorProcessId,
    [Parameter(Mandatory=$true)][string]$ExpectedLabel,
    [Parameter(Mandatory=$true)][ValidatePattern('^[A-Za-z0-9 -]{1,100}$')][string]$NewLabel
)
# A1.1's selected/expanded DisplayLabel row. This is an explicit GUI operation,
# not a serializer/file edit or a general-purpose editor automation framework.
Set-StrictMode -Version Latest
$ErrorActionPreference='Stop'
Add-Type -AssemblyName UIAutomationClient
Add-Type -AssemblyName UIAutomationTypes
Add-Type -AssemblyName System.Windows.Forms
if (-not ('RatAuthoringLabelInput' -as [type])) {
Add-Type -TypeDefinition @'
using System;using System.Runtime.InteropServices;
public static class RatAuthoringLabelInput {
 [StructLayout(LayoutKind.Sequential)]public struct Point{public int X,Y;}
 [DllImport("user32.dll")]public static extern bool SetForegroundWindow(IntPtr h);
 [DllImport("user32.dll")]public static extern IntPtr GetForegroundWindow();
 [DllImport("user32.dll")]public static extern bool SetCursorPos(int x,int y);
 [DllImport("user32.dll")]public static extern IntPtr WindowFromPoint(Point p);
 [DllImport("user32.dll")]public static extern uint GetWindowThreadProcessId(IntPtr h,out uint id);
 [DllImport("user32.dll")]public static extern void mouse_event(uint f,uint x,uint y,uint d,UIntPtr e);
}
'@
}
$process=Get-Process -Id $EditorProcessId
if($process.ProcessName -ne 'Stride.GameStudio'){throw 'Expected our Stride.GameStudio PID.'}
$handle=$process.MainWindowHandle
$window=[Windows.Automation.AutomationElement]::FromHandle($handle)
function Find-LabelRow([string]$Label) {
    $window.FindFirst([Windows.Automation.TreeScope]::Descendants,
        [Windows.Automation.PropertyCondition]::new([Windows.Automation.AutomationElement]::NameProperty,"DisplayLabel: [$Label]"))
}
$row=Find-LabelRow $ExpectedLabel
if(-not $row -or $row.Current.IsOffscreen){throw 'Select the fixture entity and expand its DisplayLabel property first.'}
$bounds=$row.Current.BoundingRectangle
$point=[RatAuthoringLabelInput+Point]::new()
$point.X=[int]($bounds.X+$bounds.Width*.75);$point.Y=[int]($bounds.Y+$bounds.Height*.55)
[RatAuthoringLabelInput]::SetForegroundWindow($handle)|Out-Null
[RatAuthoringLabelInput]::SetCursorPos($point.X,$point.Y)|Out-Null
Start-Sleep -Milliseconds 100
[uint32]$targetId=0
[RatAuthoringLabelInput]::GetWindowThreadProcessId([RatAuthoringLabelInput]::WindowFromPoint($point),[ref]$targetId)|Out-Null
if($targetId -ne $EditorProcessId -or [RatAuthoringLabelInput]::GetForegroundWindow() -ne $handle){throw 'Our editor is not under the pointer and foreground.'}
[RatAuthoringLabelInput]::mouse_event(2,0,0,0,[UIntPtr]::Zero)
[RatAuthoringLabelInput]::mouse_event(4,0,0,0,[UIntPtr]::Zero)
Start-Sleep -Milliseconds 100
foreach($keys in @("{HOME}{DELETE $($ExpectedLabel.Length)}",$NewLabel,'{ENTER}')) {
    if([RatAuthoringLabelInput]::GetForegroundWindow() -ne $handle){throw 'Editor lost foreground; stopped typing.'}
    [Windows.Forms.SendKeys]::SendWait($keys)
}
$deadline=[DateTime]::UtcNow.AddSeconds(3)
while(-not (Find-LabelRow $NewLabel) -and [DateTime]::UtcNow -lt $deadline){Start-Sleep -Milliseconds 50}
if(-not (Find-LabelRow $NewLabel)){throw 'GUI did not commit the exact expected label; inspect before saving.'}
Write-Output "GUI DisplayLabel: '$ExpectedLabel' -> '$NewLabel'. Save/undo/redo remain explicit editor actions."
