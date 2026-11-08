Add-Type -AssemblyName System.Drawing
$TaskRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$AssetDir = Join-Path $TaskRoot 'Content/Asset/WorldArtV3/Source'
New-Item -ItemType Directory -Force -Path $AssetDir | Out-Null
$SourcePath = 'C:/Users/wssh0/.codex/generated_images/01a14e2b-6aff-76e0-b68a-a4e91adb82de/exec-6ef86a91-7d9b-4aaf-aaa8-28885dbde552.png'
Copy-Item -LiteralPath $SourcePath -Destination (Join-Path $AssetDir 'Shop-original.png')
$Source = [System.Drawing.Bitmap]::FromFile($SourcePath)
$Left = $Source.Width
$Top = $Source.Height
$Right = 0
$Bottom = 0
for ($Y = 0; $Y -lt $Source.Height; $Y++) {
    for ($X = 0; $X -lt $Source.Width; $X++) {
        if ($Source.GetPixel($X, $Y).A -gt 127) {
            $Left = [Math]::Min($Left, $X)
            $Top = [Math]::Min($Top, $Y)
            $Right = [Math]::Max($Right, $X)
            $Bottom = [Math]::Max($Bottom, $Y)
        }
    }
}
$Output = [System.Drawing.Bitmap]::new(128, 96, [System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
$Graphics = [System.Drawing.Graphics]::FromImage($Output)
$Graphics.CompositingMode = [System.Drawing.Drawing2D.CompositingMode]::SourceCopy
$Graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
$Graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
$Graphics.Clear([System.Drawing.Color]::Transparent)
$Graphics.DrawImage($Source, [System.Drawing.Rectangle]::new(0,0,128,96), $Left, $Top, $Right-$Left+1, $Bottom-$Top+1, [System.Drawing.GraphicsUnit]::Pixel)
$Graphics.Dispose()
$Output.Save((Join-Path $AssetDir 'Shop.png'), [System.Drawing.Imaging.ImageFormat]::Png)
$Output.Dispose()
$Source.Dispose()
Write-Output 'Prepared 128x96 shop art with transparent alpha.'
