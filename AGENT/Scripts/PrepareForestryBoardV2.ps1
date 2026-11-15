param([string]$GeneratedRoot = 'C:/Users/wssh0/.codex/generated_images/01a14e2b-6aff-76e0-b68a-a4e91adb82de')
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$ProjectRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$Folder = Join-Path $ProjectRoot 'Content/Asset/ForestryBoardV2/Source'
New-Item -ItemType Directory -Force -Path $Folder | Out-Null
$Originals = @{
    board='exec-9d8a61a1-e36b-4251-837a-8a96aedccf43.png'
    paper='exec-9ff3b7db-e43c-4ea3-85f0-1abe45167df5.png'
    fence='exec-2a1d7997-4198-4d81-923e-5e0971958d6c.png'
    lamps='exec-f14dbfbe-756b-4e4a-9048-b25184664ff8.png'
}
foreach ($Key in $Originals.Keys) { Copy-Item -LiteralPath (Join-Path $GeneratedRoot $Originals[$Key]) -Destination (Join-Path $Folder ($Key+'-original.png')) }
Add-Type -ReferencedAssemblies @([System.Drawing.Bitmap].Assembly.Location,[System.Drawing.Rectangle].Assembly.Location) -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Drawing2D;
using System.Drawing.Imaging;
public static class ForestryBoardProcessor {
    public static void Prepare(string source, string output, int regionX, int regionY, int regionWidth, int regionHeight, int width, int height, bool whiteMatte, bool fit) {
        using (Bitmap atlas = new Bitmap(source)) {
            if (regionWidth == 0) {regionWidth = atlas.Width; regionHeight = atlas.Height;}
            int left=regionWidth, top=regionHeight, right=-1, bottom=-1;
            for (int y=0;y<regionHeight;++y) for(int x=0;x<regionWidth;++x) {
                Color c=atlas.GetPixel(regionX+x,regionY+y);
                if (c.A<200 || (whiteMatte && c.R>240 && c.G>240 && c.B>240)) continue;
                left=Math.Min(left,x); top=Math.Min(top,y); right=Math.Max(right,x); bottom=Math.Max(bottom,y);
            }
            if (right<left) throw new Exception("Empty image");
            using(Bitmap crop=atlas.Clone(new Rectangle(regionX+left,regionY+top,right-left+1,bottom-top+1),PixelFormat.Format32bppArgb))
            using(Bitmap target=new Bitmap(width,height,PixelFormat.Format32bppArgb)) {
                for(int y=0;y<crop.Height;++y) for(int x=0;x<crop.Width;++x) {
                    Color c=crop.GetPixel(x,y);
                    crop.SetPixel(x,y,c.A<200 || (whiteMatte && c.R>240 && c.G>240 && c.B>240) ? Color.Transparent : Color.FromArgb(255,c.R,c.G,c.B));
                }
                int dw=width, dh=height;
                if(fit) {double scale=Math.Min((double)width/crop.Width,(double)height/crop.Height);dw=Math.Max(1,(int)Math.Round(crop.Width*scale));dh=Math.Max(1,(int)Math.Round(crop.Height*scale));}
                using(Graphics g=Graphics.FromImage(target)) {
                    g.Clear(Color.Transparent);g.CompositingMode=CompositingMode.SourceCopy;g.InterpolationMode=InterpolationMode.NearestNeighbor;g.PixelOffsetMode=PixelOffsetMode.Half;
                    g.DrawImage(crop,new Rectangle((width-dw)/2,height-dh,dw,dh),0,0,crop.Width,crop.Height,GraphicsUnit.Pixel);
                }
                target.Save(output,ImageFormat.Png);
            }
        }
    }
}
'@
foreach ($Entry in @(@{name='BountyBoard';source='board';w=768;h=432;matte=$false},@{name='BountyPaper';source='paper';w=320;h=192;matte=$true},@{name='LumberyardEnclosure';source='fence';w=768;h=20;matte=$false})) {
    [ForestryBoardProcessor]::Prepare((Join-Path $Folder ($Entry.source+'-original.png')),(Join-Path $Folder ($Entry.name+'.png')),0,0,0,0,$Entry.w,$Entry.h,$Entry.matte,$false)
}
$LampFile=Join-Path $Folder 'lamps-original.png'
$Lamp=[Drawing.Bitmap]::new($LampFile);$Half=[int]($Lamp.Width/2);$Height=$Lamp.Height;$Lamp.Dispose()
[ForestryBoardProcessor]::Prepare($LampFile,(Join-Path $Folder 'TreeStatusGreen.png'),0,0,$Half,$Height,16,20,$false,$true)
[ForestryBoardProcessor]::Prepare($LampFile,(Join-Path $Folder 'TreeStatusRed.png'),$Half,0,$Half,$Height,16,20,$false,$true)
[ForestryBoardProcessor]::Prepare((Join-Path $ProjectRoot 'Content/Asset/MineRevisionV1/Source/SupportBeam.png'),(Join-Path $Folder 'SupportBeam.png'),0,0,0,0,32,32,$false,$false)
[ForestryBoardProcessor]::Prepare((Join-Path $ProjectRoot 'Content/Asset/LumberyardV1/Source/Lumberyard.png'),(Join-Path $Folder 'Lumberyard.png'),0,0,0,0,192,124,$false,$false)
Write-Output 'Prepared separate board, opaque paper, grounded fence, faceted lamps and fitted supports.'
