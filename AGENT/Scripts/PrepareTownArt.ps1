#Requires -PSEdition Desktop
param([string]$SourcePath = '')
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$OutputFolder = Join-Path $ProjectRoot 'Content/Asset/TownArtV1/Source'
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
public static class TownArtProcessor {
    public static void Prepare(string source, string output, int column, int row, int width, int height) {
        using (Bitmap atlas = new Bitmap(source)) {
            int cellWidth = atlas.Width / 3, cellHeight = atlas.Height / 3;
            int left = cellWidth, top = cellHeight, right = -1, bottom = -1;
            for (int y = 0; y < cellHeight; ++y) for (int x = 0; x < cellWidth; ++x) {
                if (atlas.GetPixel(column * cellWidth + x, row * cellHeight + y).A < 232) continue;
                left = Math.Min(left, x); top = Math.Min(top, y); right = Math.Max(right, x); bottom = Math.Max(bottom, y);
            }
            if (right < left) throw new Exception("Empty sprite cell");
            Rectangle bounds = new Rectangle(column * cellWidth + left, row * cellHeight + top, right - left + 1, bottom - top + 1);
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
    @{ name = 'Elevator'; column = 0; row = 0; width = 64; height = 96 },
    @{ name = 'RailFactory'; column = 1; row = 0; width = 128; height = 96 },
    @{ name = 'HouseA'; column = 2; row = 0; width = 96; height = 96 },
    @{ name = 'HouseB'; column = 0; row = 1; width = 96; height = 96 },
    @{ name = 'MineCart'; column = 1; row = 1; width = 48; height = 32 },
    @{ name = 'AutoMineCart'; column = 2; row = 1; width = 48; height = 32 },
    @{ name = 'Rail'; column = 0; row = 2; width = 32; height = 8 },
    @{ name = 'RailEnd'; column = 1; row = 2; width = 8; height = 8 },
    @{ name = 'SteadyBelt'; column = 2; row = 2; width = 24; height = 24 }
)
$Manifest = foreach ($Sprite in $Sprites) {
    $FileName = $Sprite.name + '.png'
    [TownArtProcessor]::Prepare($SourcePath, (Join-Path $OutputFolder $FileName), $Sprite.column, $Sprite.row, $Sprite.width, $Sprite.height)
    @{ name = $Sprite.name; width = $Sprite.width; height = $Sprite.height; file = 'Content/Asset/TownArtV1/Source/' + $FileName }
}
$Encoding = [System.Text.UTF8Encoding]::new($true)
[System.IO.File]::WriteAllText((Join-Path $OutputFolder 'manifest.json'), ($Manifest | ConvertTo-Json -Depth 4) + "`r`n", $Encoding)
Write-Output 'Prepared nine town and transport sprites at native pixel sizes.'
