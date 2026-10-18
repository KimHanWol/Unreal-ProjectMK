$EngineDir = 'C:\Program Files\Epic Games\UE_5.8\Engine'
$ProjectDir = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$ProjectFile = Join-Path $ProjectDir 'ProjectMK.uproject'
$EditorExe = Join-Path $EngineDir 'Binaries\Win64\UnrealEditor.exe'
$BuildBat = Join-Path $EngineDir 'Build\BatchFiles\Build.bat'
$McpPort = 8000
$LogDir = Join-Path $ProjectDir 'Saved\AgentLogs'
New-Item -ItemType Directory -Force $LogDir | Out-Null

function Get-ProjectEditorProcess {
    Get-CimInstance Win32_Process -Filter "Name = 'UnrealEditor.exe'" |
        Where-Object { $_.CommandLine -like '*ProjectMK.uproject*' }
}

function Test-McpPort {
    try { Get-NetTCPConnection -State Listen -LocalPort $McpPort -ErrorAction Stop | Out-Null; return $true }
    catch { return $false }
}
