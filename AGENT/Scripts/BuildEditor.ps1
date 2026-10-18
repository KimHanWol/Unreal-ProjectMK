# Builds the ProjectMKEditor target. The editor must be closed (Live Coding locks the DLLs).
. "$PSScriptRoot\EditorCommon.ps1"

if (Get-ProjectEditorProcess) {
    Write-Output 'BUILD_SKIPPED: editor is running. Run StopEditor.ps1 first.'
    exit 2
}

$LogFile = Join-Path $LogDir 'Build.log'
& $BuildBat ProjectMKEditor Win64 Development "-Project=$ProjectFile" -WaitMutex 2>&1 | Out-File -Encoding utf8 $LogFile
$ExitCode = $LASTEXITCODE

Get-Content $LogFile | Select-String -Pattern 'error|warning C|Result:|Total execution time' | Select-Object -Last 40 | ForEach-Object { $_.Line }
Write-Output "BUILD_EXIT=$ExitCode (log: $LogFile)"
exit $ExitCode
