#Requires -PSEdition Desktop
param([string]$SourcePath = '')
$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Drawing
$MkRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$MkFolder = Join-Path $MkRoot 'Content/Asset/StorageChestV3/Source'
New-Item -ItemType Directory -Path $MkFolder -Force | Out-Null
$MkOriginal = Join-Path $MkFolder 'chest-original.png'
if ([string]::IsNullOrWhiteSpace($SourcePath)) { $SourcePath = $MkOriginal }
if ([System.IO.Path]::GetFullPath($SourcePath) -ne [System.IO.Path]::GetFullPath($MkOriginal)) { Copy-Item -LiteralPath $SourcePath -Destination $MkOriginal }
Add-Type -ReferencedAssemblies @([System.Drawing.Bitmap].Assembly.Location, [System.Drawing.Rectangle].Assembly.Location) -TypeDefinition @'
using System;
using System.Drawing;
using System.Drawing.Imaging;
public static class StorageChestPixels {
    public static void Prepare(string input, string output) {
        using (Bitmap source = new Bitmap(input)) {
            int left=source.Width, top=source.Height, right=-1, bottom=-1;
            for(int y=0;y<source.Height;y++) for(int x=0;x<source.Width;x++) {
                if(source.GetPixel(x,y).A<232)continue;
                left=Math.Min(left,x);top=Math.Min(top,y);right=Math.Max(right,x);bottom=Math.Max(bottom,y);
            }
            if(right<left)throw new Exception("Chest sprite is empty");
            float scale=Math.Min(28f/(right-left+1),22f/(bottom-top+1));
            int width=(int)Math.Round((right-left+1)*scale),height=(int)Math.Round((bottom-top+1)*scale);
            using(Bitmap target=new Bitmap(32,32,PixelFormat.Format32bppArgb)) {
                for(int y=0;y<height;y++)for(int x=0;x<width;x++) {
                    int sx=left+Math.Min(right-left,(int)((x+0.5f)*(right-left+1)/width));
                    int sy=top+Math.Min(bottom-top,(int)((y+0.5f)*(bottom-top+1)/height));
                    Color c=source.GetPixel(sx,sy);
                    if(c.A>=232)target.SetPixel((32-width)/2+x,32-height+y,Color.FromArgb(255,c.R,c.G,c.B));
                }
                target.Save(output,ImageFormat.Png);
            }
        }
    }
}
'@
[StorageChestPixels]::Prepare($MkOriginal, (Join-Path $MkFolder 'StorageChest.png'))
Write-Output 'Prepared a native 32x32 closed storage chest with hard alpha.'
