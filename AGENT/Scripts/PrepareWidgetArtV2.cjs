// Split the generated atlas into native UI roles; keep original artwork and hard alpha.
const fs = require('fs');
const path = require('path');
const sharp = require('C:/Users/wssh0/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '../..');
const dir = path.join(root, 'Content/Asset/WidgetArtV2/Source');
const specs = [
  ['ActionPrimary',128,40,16,12], ['ActionSecondary',128,40,16,12],
  ['ActionDanger',128,40,16,12], ['QuantityPreset',96,32,12,10],
  ['Stepper',32,32,10,10], ['QuantityPresetSelected',96,32,12,10],
  ['InputWell',128,40,12,12], ['SaleTotal',128,40,12,12],
  ['MerchantMat',160,88,24,24], ['RecipeRow',192,64,56,16],
  ['InventoryDrawer',160,88,16,20], ['SectionDivider',192,16,80,0],
  ['IconSatchel',48,48,0,0], ['IconWorkbench',64,64,0,0],
  ['IconTrade',48,48,0,0], ['IconStorage',48,40,0,0],
];
async function main() {
  fs.mkdirSync(path.join(dir,'UI'),{recursive:true});
  const meta = await sharp(path.join(dir,'component-atlas.png')).metadata();
  if (!meta.hasAlpha) throw new Error('Generated atlas must have alpha.');
  const manifest=[];
  for (let n=0;n<specs.length;n++) {
    const [name,w,h,mx,my]=specs[n];
    const left=Math.round((n%4)*meta.width/4), top=Math.round(Math.floor(n/4)*meta.height/4);
    const cellW=Math.round((n%4+1)*meta.width/4)-left, cellH=Math.round((Math.floor(n/4)+1)*meta.height/4)-top;
    const raw=await sharp(path.join(dir,'component-atlas.png')).extract({left,top,width:cellW,height:cellH}).ensureAlpha().raw().toBuffer();
    let x0=cellW,y0=cellH,x1=-1,y1=-1;
    for(let y=0;y<cellH;y++) for(let x=0;x<cellW;x++) {
      const i=(y*cellW+x)*4;
      raw[i+3]=raw[i+3]>=180?255:0;
      if(raw[i+3]){x0=Math.min(x0,x);y0=Math.min(y0,y);x1=Math.max(x1,x);y1=Math.max(y1,y);}
    }
    if(x1<x0)throw new Error('Empty atlas cell: '+name);
    const file='Content/Asset/WidgetArtV2/Source/UI/'+name+'.png';
    await sharp(raw,{raw:{width:cellW,height:cellH,channels:4}}).extract({left:x0,top:y0,width:x1-x0+1,height:y1-y0+1}).resize(w,h,{fit:'fill',kernel:'nearest'}).png().toFile(path.join(root,file));
    manifest.push({name,file,width:w,height:h,marginX:mx,marginY:my,cell:n,atlasBounds:[x0,y0,x1,y1]});
  }
  fs.writeFileSync(path.join(dir,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
  console.log(JSON.stringify({atlas:[meta.width,meta.height],alpha:meta.hasAlpha,assets:manifest.length}));
}
main().catch(e=>{console.error(e);process.exit(1);});
