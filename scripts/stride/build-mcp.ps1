[CmdletBinding()]
param([string]$CheckoutPath)
. (Join-Path $PSScriptRoot 'common.ps1')
$engine=Get-StrideCheckoutPath $CheckoutPath
$head=Assert-StrideCheckout $engine
if($head -ne (Get-StrideIntegrationCommit)){throw 'MCP requires exact pinned Stride integration commit.'}
$repository=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '../..'))
$output=Join-Path $repository 'build/stride-mcp'
$editorBin=Join-Path $engine 'sources/editor/Stride.GameStudio/bin/Release/net10.0-windows'
New-Item -ItemType Directory -Force -Path $output | Out-Null
foreach($name in @('Hook','Adapter','Server')){
    $project=Join-Path $repository "tools/stride-mcp/Rat.StrideMcp.$name/Rat.StrideMcp.$name.csproj"
    & dotnet build $project -c Release --nologo -p:RestoreLockedMode=true "-p:StrideBin=$editorBin"
    if($LASTEXITCODE -ne 0){throw "MCP $name build failed."}
}
$adapter=Join-Path $output 'adapter'
New-Item -ItemType Directory -Force -Path $adapter | Out-Null
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Hook/bin/Release/net10.0/Rat.StrideMcp.Hook.dll') -Destination $adapter
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Adapter/bin/Release/net10.0-windows/Rat.StrideMcp.Adapter.dll') -Destination $adapter
$server=Join-Path $output 'server'
& dotnet publish (Join-Path $repository 'tools/stride-mcp/Rat.StrideMcp.Server/Rat.StrideMcp.Server.csproj') -c Release --no-restore -o $server
if($LASTEXITCODE -ne 0){throw 'MCP server publish failed.'}
Copy-Item -LiteralPath (Join-Path $repository 'tools/stride-mcp/NOTICE'),(Join-Path $repository 'tools/stride-mcp/LICENSE-AkerMCP') -Destination $output
Write-Host "MCP artifacts: $output"
