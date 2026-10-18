# Closes the ProjectMK editor. Tries a graceful close first, then force-kills after a timeout.
param([int]$TimeoutSeconds = 30)
. "$PSScriptRoot\EditorCommon.ps1"

$Editors = Get-ProjectEditorProcess
if (-not $Editors) { Write-Output 'EDITOR_NOT_RUNNING'; exit 0 }

foreach ($Editor in $Editors) {
    $Process = Get-Process -Id $Editor.ProcessId -ErrorAction SilentlyContinue
    if ($Process) { $Process.CloseMainWindow() | Out-Null }
}

$Deadline = (Get-Date).AddSeconds($TimeoutSeconds)
while ((Get-Date) -lt $Deadline -and (Get-ProjectEditorProcess)) { Start-Sleep -Seconds 1 }

$Remaining = Get-ProjectEditorProcess
if ($Remaining) {
    $Remaining | ForEach-Object { Stop-Process -Id $_.ProcessId -Force }
    Write-Output 'EDITOR_FORCE_KILLED'
} else {
    Write-Output 'EDITOR_CLOSED'
}
