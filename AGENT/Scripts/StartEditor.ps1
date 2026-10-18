# Launches the editor with the MCP server enabled and waits until the MCP port is listening.
param([int]$TimeoutSeconds = 600, [string]$Map = '/Game/Level/Main')
. "$PSScriptRoot\EditorCommon.ps1"

if (Get-ProjectEditorProcess) {
    Write-Output 'EDITOR_ALREADY_RUNNING'
} else {
    $EditorArgs = @("`"$ProjectFile`"", $Map, '-ModelContextProtocolStartServer', "-ModelContextProtocolPort=$McpPort", '-NoSplash')
    Start-Process -FilePath $EditorExe -ArgumentList $EditorArgs | Out-Null
    Write-Output 'EDITOR_LAUNCHED'
}

$Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $Deadline) {
    if (Test-McpPort) { Write-Output "MCP_READY on port $McpPort"; exit 0 }
    if (-not (Get-ProjectEditorProcess)) { Write-Output 'EDITOR_EXITED before MCP became ready (check Saved\Logs\ProjectMK.log)'; exit 1 }
    Start-Sleep -Seconds 3
}
Write-Output 'MCP_TIMEOUT'
exit 1
