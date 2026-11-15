#Requires -PSEdition Desktop
param([string]$SourceRoot = 'C:/Users/wssh0/.codex/generated_images/01a14e2b-6aff-76e0-b68a-a4e91adb82de')
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Drawing
$ProjectRoot=Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$OutputFolder=Join-Path $ProjectRoot 'Content/Asset/MineRevisionV1/Source'
New-Item -ItemType Directory -Path $OutputFolder -Force | Out-Null
$Originals=@{
    transport='exec-6815bfe6-d711-4396-ad6f-b053f3ee587b.png'
    yard='exec-0f94a941-27ba-461f-85c4-14d8899d4ffb.png'
    leaves='exec-6bc2b70a-9d6c-4bf1-91da-c0eb82297f10.png'
    board='exec-4ede20b5-9b55-496f-a565-c4cbf00c1a74.png'
    forest='exec-1c88fbfd-ce36-4c74-b311-b0536ffac64b.png'
    trestle='exec-92f9d19d-ff9f-4eae-9540-0d5eb0c2d2f5.png'
}
foreach($Key in $Originals.Keys) {
    $Stored=Join-Path $OutputFolder ($Key+'-original.png')
    $InputPath=Join-Path $SourceRoot $Originals[$Key]
    if(Test-Path -LiteralPath $InputPath) { if([IO.Path]::GetFullPath($Stored) -ne [IO.Path]::GetFullPath($InputPath)) { Copy-Item -LiteralPath $InputPath -Destination $Stored } }
    if(!(Test-Path -LiteralPath $Stored)) { throw ('Missing original '+$Key) }
}
Add-Type -ReferencedAssemblies @([System.Drawing.Bitmap].Assembly.Location, [System.Drawing.Rectangle].Assembly.Location) -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
public static class MineRevisionProcessor {
    public static void Prepare(string source, string output, int regionX, int regionY, int regionWidth, int regionHeight, int width, int height) {
        using (Bitmap atlas = new Bitmap(source)) {
            int cellWidth = regionWidth, cellHeight = regionHeight;
            int left = cellWidth, top = cellHeight, right = -1, bottom = -1;
            for (int y = 0; y < cellHeight; ++y) for (int x = 0; x < cellWidth; ++x) {
                if (atlas.GetPixel(regionX + x, regionY + y).A < 232 || (atlas.GetPixel(regionX + x, regionY + y).R > 235 && atlas.GetPixel(regionX + x, regionY + y).G > 235 && atlas.GetPixel(regionX + x, regionY + y).B > 235)) continue;
                left = Math.Min(left, x); top = Math.Min(top, y); right = Math.Max(right, x); bottom = Math.Max(bottom, y);
            }
            if (right < left) throw new Exception("Empty sprite cell");
            Rectangle bounds = new Rectangle(regionX + left, regionY + top, right - left + 1, bottom - top + 1);
            using (Bitmap cropped = atlas.Clone(bounds, PixelFormat.Format32bppArgb))
            using (Bitmap target = new Bitmap(width, height, PixelFormat.Format32bppArgb)) {
                for (int y = 0; y < cropped.Height; ++y) for (int x = 0; x < cropped.Width; ++x) {
                    Color color = cropped.GetPixel(x,y);
                    cropped.SetPixel(x,y,(color.A < 232 || (color.R > 235 && color.G > 235 && color.B > 235)) ? Color.Transparent : Color.FromArgb(255,color.R,color.G,color.B));
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


$Entries=@(
    @{name='SupportBeam';source='transport';x=0;y=0;w=760;h=560;width=32;height=32},
    @{name='Rail';source='transport';x=760;y=0;w=776;h=560;width=32;height=12},
    @{name='MineCart';source='transport';x=0;y=560;w=760;h=464;width=48;height=32},
    @{name='AutoMineCart';source='transport';x=760;y=560;w=776;h=464;width=48;height=32},
    @{name='RailTrestleDown';source='trestle';x=0;y=0;w=880;h=887;width=32;height=32},
    @{name='RailTrestleUp';source='trestle';x=890;y=0;w=884;h=887;width=32;height=32}
)
foreach($Entry in $Entries) { [MineRevisionProcessor]::Prepare((Join-Path $OutputFolder ($Entry.source+'-original.png')),(Join-Path $OutputFolder ($Entry.name+'.png')),$Entry.x,$Entry.y,$Entry.w,$Entry.h,$Entry.width,$Entry.height) }
foreach($Entry in @(@{name='LumberyardEnclosure';source='yard';width=768;height=80},@{name='TreeLeaves';source='leaves';width=32;height=32},@{name='BountyBoard';source='board';width=512;height=384},@{name='ForestBackdrop';source='forest';width=768;height=256})) {
    $Original=Join-Path $OutputFolder ($Entry.source+'-original.png')
    $Pic=[Drawing.Bitmap]::new($Original);$W=$Pic.Width;$H=$Pic.Height;$Pic.Dispose()
    [MineRevisionProcessor]::Prepare($Original,(Join-Path $OutputFolder ($Entry.name+'.png')),0,0,$W,$H,$Entry.width,$Entry.height)
}
Write-Output 'Prepared native block, transport, long enclosure, woodland and noticeboard sprites.'
