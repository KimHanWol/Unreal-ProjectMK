param([string]$GeneratedRoot = 'C:/Users/wssh0/.codex/generated_images/01a14e2b-6aff-76e0-b68a-a4e91adb82de')
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Folder = Join-Path $ProjectRoot 'Content/Asset/TreeArtV3/Source'
New-Item -ItemType Directory -Path $Folder -Force | Out-Null
$Originals = @{
    TreeTrunk='exec-effa1882-8bc5-4cc6-a1bd-47d36fd21965.png'
    TreeLeaves='exec-1678768e-b2ca-4dd2-ab7f-d8d0d83bdb8e.png'
}
foreach ($Name in $Originals.Keys) {
    $Original = Join-Path $Folder ($Name+'-original.png')
    Copy-Item -LiteralPath (Join-Path $GeneratedRoot $Originals[$Name]) -Destination $Original
    $Source = [Drawing.Bitmap]::new($Original)
    $Target = [Drawing.Bitmap]::new(32,32,[Drawing.Imaging.PixelFormat]::Format32bppArgb)
    $Graphics = [Drawing.Graphics]::FromImage($Target)
    $Graphics.CompositingMode = [Drawing.Drawing2D.CompositingMode]::SourceCopy
    $Graphics.InterpolationMode = [Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
    $Graphics.PixelOffsetMode = [Drawing.Drawing2D.PixelOffsetMode]::Half
    $Graphics.DrawImage($Source,[Drawing.Rectangle]::new(0,0,32,32),0,0,$Source.Width,$Source.Height,[Drawing.GraphicsUnit]::Pixel)
    $Graphics.Dispose(); $Source.Dispose()
    for ($Y=0; $Y -lt 32; ++$Y) { for ($X=0; $X -lt 32; ++$X) {
        $Color=$Target.GetPixel($X,$Y)
        $HardColor=if($Color.A -lt 180){[Drawing.Color]::Transparent}else{[Drawing.Color]::FromArgb(255,$Color.R,$Color.G,$Color.B)}
        $Target.SetPixel($X,$Y,$HardColor)
    } }
    $Target.Save((Join-Path $Folder ($Name+'.png')),[Drawing.Imaging.ImageFormat]::Png)
    $Target.Dispose()
}
Write-Output 'Prepared native 32x32 tree textures with hard alpha and foliage gaps.'
