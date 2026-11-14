#Requires -PSEdition Desktop
param([string]$SourcePath = '')
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$OutputFolder = Join-Path $ProjectRoot 'Content/Asset/TownArtV2/Source'
New-Item -ItemType Directory -Path $OutputFolder -Force | Out-Null
$AtlasPath = Join-Path $OutputFolder 'atlas-original.png'
if ([string]::IsNullOrWhiteSpace($SourcePath)) { $SourcePath = $AtlasPath }
if ([System.IO.Path]::GetFullPath($SourcePath) -ne [System.IO.Path]::GetFullPath($AtlasPath)) {
    Copy-Item -LiteralPath $SourcePath -Destination $AtlasPath
}
Add-Type -ReferencedAssemblies @([System.Drawing.Bitmap].Assembly.Location, [System.Drawing.Rectangle].Assembly.Location) -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
public static class TownArtV2Processor {
    public static void Prepare(string source, string output, int regionX, int regionY, int regionWidth, int regionHeight, int width, int height) {
        using (Bitmap atlas = new Bitmap(source)) {
            int cellWidth = regionWidth, cellHeight = regionHeight;
            int left = cellWidth, top = cellHeight, right = -1, bottom = -1;
            for (int y = 0; y < cellHeight; ++y) for (int x = 0; x < cellWidth; ++x) {
                if (atlas.GetPixel(regionX + x, regionY + y).A < 232) continue;
                left = Math.Min(left, x); top = Math.Min(top, y); right = Math.Max(right, x); bottom = Math.Max(bottom, y);
            }
            if (right < left) throw new Exception("Empty sprite cell");
            Rectangle bounds = new Rectangle(regionX + left, regionY + top, right - left + 1, bottom - top + 1);
            using (Bitmap cropped = atlas.Clone(bounds, PixelFormat.Format32bppArgb))
            using (Bitmap target = new Bitmap(width, height, PixelFormat.Format32bppArgb)) {
                for (int y = 0; y < cropped.Height; ++y) for (int x = 0; x < cropped.Width; ++x) {
                    Color color = cropped.GetPixel(x,y);
                    cropped.SetPixel(x,y,color.A < 232 ? Color.Transparent : Color.FromArgb(255,color.R,color.G,color.B));
                }
                double scale = Math.Min((double)width / cropped.Width, (double)height / cropped.Height);
                int drawWidth = Math.Max(1,(int)Math.Round(cropped.Width*scale)), drawHeight = Math.Max(1,(int)Math.Round(cropped.Height*scale));
                using (Graphics graphics = Graphics.FromImage(target)) {
                    graphics.Clear(Color.Transparent); graphics.CompositingMode = CompositingMode.SourceCopy;
                    graphics.InterpolationMode = InterpolationMode.NearestNeighbor; graphics.PixelOffsetMode = PixelOffsetMode.Half;
                    graphics.DrawImage(cropped,new Rectangle((width-drawWidth)/2,height-drawHeight,drawWidth,drawHeight),0,0,cropped.Width,cropped.Height,GraphicsUnit.Pixel);
                }
                target.Save(output,ImageFormat.Png);
            }
        }
    }
}
'@
$Sprites = @(
    @{ name = 'Elevator'; x = 0; y = 0; regionWidth = 512; regionHeight = 568; width = 192; height = 224 },
    @{ name = 'RailFactory'; x = 512; y = 0; regionWidth = 576; regionHeight = 568; width = 288; height = 192 },
    @{ name = 'HouseA'; x = 1088; y = 0; regionWidth = 448; regionHeight = 568; width = 160; height = 128 },
    @{ name = 'HouseB'; x = 0; y = 600; regionWidth = 512; regionHeight = 424; width = 160; height = 128 },
    @{ name = 'VillageBackdrop'; x = 512; y = 600; regionWidth = 576; regionHeight = 424; width = 288; height = 160 },
    @{ name = 'VillageNotice'; x = 1088; y = 600; regionWidth = 448; regionHeight = 424; width = 96; height = 96 }
)
$Manifest = foreach ($Sprite in $Sprites) {
    $FileName = $Sprite.name + '.png'
    [TownArtV2Processor]::Prepare($SourcePath, (Join-Path $OutputFolder $FileName), $Sprite.x, $Sprite.y, $Sprite.regionWidth, $Sprite.regionHeight, $Sprite.width, $Sprite.height)
    @{ name = $Sprite.name; width = $Sprite.width; height = $Sprite.height; file = 'Content/Asset/TownArtV2/Source/' + $FileName }
}
$Encoding = [System.Text.UTF8Encoding]::new($true)
[System.IO.File]::WriteAllText((Join-Path $OutputFolder 'manifest.json'), ($Manifest | ConvertTo-Json -Depth 4) + "`r`n", $Encoding)
Write-Output 'Prepared six expanded village and mining facility sprites at native pixel sizes.'
