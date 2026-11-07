// Slice the generated skin atlas into role-specific resolutions, preserving hard alpha.
const fs = require('fs');
const path = require('path');
const sharp = require('C:/Users/wssh0/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '../..');
const source = path.join(root, 'Content/Asset/WidgetArtV1/Source');
const specs = [
  ['Panel',5,85,330,255,160,120,16,16],
  ['InsetPanel',340,135,280,185,128,80,8,8],
  ['WindowTitle',630,185,305,80,160,32,24,10],
  ['RecipeCard',943,185,300,80,160,32,24,8],
  ['Button',15,460,303,110,96,32,12,10],
  ['ButtonHover',330,460,295,110,96,32,12,10],
  ['ButtonPressed',635,460,295,110,96,32,12,10],
  ['ButtonDisabled',945,460,295,110,96,32,12,10],
  ['Slot',60,670,210,210,48,48,8,8],
  ['SelectedSlot',315,670,220,210,48,48,8,8],
  ['Quickbar',545,710,410,125,192,48,28,12],
  ['Tooltip',975,670,260,210,128,96,14,14],
  ['BarFrame',15,1000,315,90,160,32,36,10],
  ['OxygenFrame',335,1000,310,90,160,32,36,10],
  ['Minimap',670,920,255,245,96,96,18,18],
  ['Keycap',1020,975,150,150,24,24,5,5],
];
async function main() {
  fs.mkdirSync(path.join(source,'UI'),{recursive:true});
  const manifest=[];
  const previews=[];
  for(const [name,left,top,width,height,w,h,mx,my] of specs) {
    const raw=await sharp(path.join(source,'skin-atlas.png')).extract({left,top,width,height}).ensureAlpha().raw().toBuffer();
    let x0=width,y0=height,x1=0,y1=0;
    for(let y=0;y<height;y++)for(let x=0;x<width;x++){
      const i=(y*width+x)*4;
      raw[i+3]=raw[i+3]>=160?255:0;
      if(raw[i+3]){x0=Math.min(x0,x);y0=Math.min(y0,y);x1=Math.max(x1,x);y1=Math.max(y1,y);}
    }
    if(x1<=x0||y1<=y0)throw new Error(`Empty crop: ${name}`);
    const png=await sharp(raw,{raw:{width,height,channels:4}})
      .extract({left:x0,top:y0,width:x1-x0+1,height:y1-y0+1})
      .resize(w,h,{kernel:'nearest',fit:'fill'}).png().toBuffer();
    const file=`Content/Asset/WidgetArtV1/Source/UI/${name}.png`;
    fs.writeFileSync(path.join(root,file),png);
    manifest.push({name,group:'UI',file,width:w,height:h,marginX:mx,marginY:my});
    const i=manifest.length-1;
    previews.push({input:await sharp(png).resize({width:240,height:160,fit:'inside',kernel:'nearest'}).png().toBuffer(),left:(i%4)*260+10,top:Math.floor(i/4)*180+10});
  }
  fs.writeFileSync(path.join(source,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
  fs.mkdirSync(path.join(root,'output/widget-art'),{recursive:true});
  await sharp({create:{width:1040,height:720,channels:4,background:'#343c44'}}).composite(previews).png().toFile(path.join(root,'output/widget-art/skin-preview.png'));
  console.log(`Prepared ${manifest.length} UI textures.`);
}
main().catch(error=>{console.error(error);process.exitCode=1;});
