function Stop-OwnedMcpProcess([hashtable]$Ownership){
    if($null -eq $Ownership -or -not $Ownership.ContainsKey('OwnedProcess') -or $null -eq $Ownership.OwnedProcess){return}
    $owned=$Ownership.OwnedProcess
    if($owned.HasExited){return}
    if(($Ownership.ContainsKey('StartTimeUtc') -and $owned.StartTime.ToUniversalTime() -ne $Ownership.StartTimeUtc) -or $owned.MainModule.FileName -ne $Ownership.ExecutablePath){throw 'Owned process identity changed; refusing termination.'}
    $owned.Kill()
    if(-not $owned.WaitForExit(10000)){throw 'Owned MCP editor did not exit within 10 seconds.'}
}
