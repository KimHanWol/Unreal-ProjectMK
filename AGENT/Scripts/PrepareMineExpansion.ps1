#Requires -PSEdition Desktop
param([Parameter(Mandatory=$true)][string]$ShopSource, [Parameter(Mandatory=$true)][string]$BackdropSource, [Parameter(Mandatory=$true)][string]$TreeSource)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$OutputFolder = Join-Path $ProjectRoot 'Content/Asset/MineExpansionV1/Source'
New-Item -ItemType Directory -Path $OutputFolder -Force | Out-Null
Copy-Item -LiteralPath $ShopSource -Destination (Join-Path $OutputFolder 'shop-original.png')
Copy-Item -LiteralPath $BackdropSource -Destination (Join-Path $OutputFolder 'backdrop-original.png')
Copy-Item -LiteralPath $TreeSource -Destination (Join-Path $OutputFolder 'trees-original.png')
Add-Type -ReferencedAssemblies @([System.Drawing.Bitmap].Assembly.Location, [System.Drawing.Rectangle].Assembly.Location) -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
public static class MineExpansionProcessor {
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

$Manifest = @()
foreach ($Entry in @(@{name='Shop'; source=$ShopSource; width=192; height=168}, @{name='IndustrialBackdrop'; source=$BackdropSource; width=512; height=160})) {
    $Bitmap = [System.Drawing.Bitmap]::new($Entry.source)
    $Width = $Bitmap.Width; $Height = $Bitmap.Height; $Bitmap.Dispose()
    [MineExpansionProcessor]::Prepare($Entry.source, (Join-Path $OutputFolder ($Entry.name+'.png')), 0, 0, $Width, $Height, $Entry.width, $Entry.height)
    $Manifest += @{name=$Entry.name; width=$Entry.width; height=$Entry.height; file=('Content/Asset/MineExpansionV1/Source/'+$Entry.name+'.png')}
}
$Bitmap = [System.Drawing.Bitmap]::new($TreeSource)
$Width = $Bitmap.Width; $Height = $Bitmap.Height; $Bitmap.Dispose()
$Names = @('TreeTrunk','TreeLeaves','Wood')
for ($Index=0; $Index -lt 3; ++$Index) {
    $Left=[int][Math]::Floor($Index*$Width/3); $Right=[int][Math]::Floor(($Index+1)*$Width/3)
    [MineExpansionProcessor]::Prepare($TreeSource,(Join-Path $OutputFolder ($Names[$Index]+'.png')),$Left,0,($Right-$Left),$Height,32,32)
    $Manifest += @{name=$Names[$Index]; width=32; height=32; file=('Content/Asset/MineExpansionV1/Source/'+$Names[$Index]+'.png')}
}
[System.IO.File]::WriteAllText((Join-Path $OutputFolder 'manifest.json'),($Manifest | ConvertTo-Json -Depth 4)+"`r`n",[System.Text.UTF8Encoding]::new($true))
Write-Output 'Prepared native expansion sprites.'
