#Requires -PSEdition Desktop
param([Parameter(Mandatory=$true)][string]$CanopySource, [Parameter(Mandatory=$true)][string]$PropsSource)
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$OutputFolder = Join-Path $ProjectRoot 'Content/Asset/LumberyardV1/Source'
New-Item -ItemType Directory -Path $OutputFolder -Force | Out-Null
if ([System.IO.Path]::GetFullPath($CanopySource) -ne [System.IO.Path]::GetFullPath((Join-Path $OutputFolder 'canopy-original.png'))) { Copy-Item -LiteralPath $CanopySource -Destination (Join-Path $OutputFolder 'canopy-original.png') }
if ([System.IO.Path]::GetFullPath($PropsSource) -ne [System.IO.Path]::GetFullPath((Join-Path $OutputFolder 'props-original.png'))) { Copy-Item -LiteralPath $PropsSource -Destination (Join-Path $OutputFolder 'props-original.png') }
Add-Type -ReferencedAssemblies @([System.Drawing.Bitmap].Assembly.Location, [System.Drawing.Rectangle].Assembly.Location) -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
public static class LumberyardProcessor {
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


$Pic=[System.Drawing.Bitmap]::new($CanopySource)
$CanopyWidth=$Pic.Width; $CanopyHeight=$Pic.Height; $Pic.Dispose()
[LumberyardProcessor]::Prepare($CanopySource,(Join-Path $OutputFolder 'Canopy.png'),0,0,$CanopyWidth,$CanopyHeight,160,96)
$Manifest = @()
$Entries = @(
    @{name='Lumberyard'; x=0; y=0; w=1145; h=750; width=192; height=128},
    @{name='LanternPost'; x=1150; y=0; w=386; h=750; width=48; height=112},
    @{name='PlantingBed'; x=0; y=770; w=1536; h=254; width=192; height=32}
)
foreach ($Entry in $Entries) {
    [LumberyardProcessor]::Prepare($PropsSource,(Join-Path $OutputFolder ($Entry.name+'.png')),$Entry.x,$Entry.y,$Entry.w,$Entry.h,$Entry.width,$Entry.height)
    $Manifest += @{name=$Entry.name; file=('Content/Asset/LumberyardV1/Source/'+$Entry.name+'.png')}
}
$Canopy=[System.Drawing.Bitmap]::new((Join-Path $OutputFolder 'Canopy.png'))
$Cells=@()
for($Y=0; $Y -lt 3; ++$Y) { for($X=0; $X -lt 5; ++$X) {
    $Count=0
    for($DY=0; $DY -lt 32; ++$DY) { for($DX=0; $DX -lt 32; ++$DX) { if($Canopy.GetPixel($X*32+$DX,$Y*32+$DY).A -gt 0) {++$Count} } }
    if($Count -gt 0) { $Cells += @{name=('TreeLeaves_'+$X+'_'+$Y); x=$X; y=$Y; pixels=$Count} }
} }
$Canopy.Dispose()
[System.IO.File]::WriteAllText((Join-Path $OutputFolder 'manifest.json'),(@{props=$Manifest; leaves=$Cells} | ConvertTo-Json -Depth 5)+"`r`n",[System.Text.UTF8Encoding]::new($true))
Write-Output ('Prepared continuous canopy, '+$Cells.Count+' fixed atlas regions, and three lumberyard props.')
