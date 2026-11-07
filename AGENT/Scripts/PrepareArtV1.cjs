// Deterministic atlas extraction for the generated ProjectMK art set.
const fs = require('fs');
const path = require('path');
const sharp = require('C:/Users/wssh0/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '../..');
const dest = path.join(root, 'Content/Asset/ArtV1/Source');
const generated = 'C:/Users/wssh0/.codex/generated_images/01a14e2b-6aff-76e0-b68a-a4e91adb82de';
const manifest = [];
const itemNames = ['Coin','Oxygen','Ore_Coal','Ore_Copper','Ore_Iron','Ore_Gold','Ore_Emerald','Ore_Diamond','Ore_Titanium','Ore_Lithium','Slime','Chain','OxygenCapsule','RepairKit','RecallFlare','SupportBeam','CushionGel','BlastCharge','Insurance','Ore_Test'];
const skillNames = ['DrillPower','DrillSpeed','DrillRange','HealthBoost','OxygenTank','Magnet','BagUpgrade','MoveSpeed','FlyingSpeed','HealthRegen','ChainBreak','Recycler','AirPocket','FieldRepair','Insurance','LuckyStrike','LastOxygen','CoinMaster','Scavenger','MinerDiscipline','RecklessDig','SpeedRunner','HeavyCarrier','Upgrade','Lantern'];

function sourceAtlas(filename, label) {
  const copy=path.join(dest,'Atlases',label+'.png');
  if(!fs.existsSync(copy)) {
    fs.mkdirSync(path.dirname(copy),{recursive:true});
    fs.copyFileSync(path.join(generated,filename),copy);
  }
  return copy;
}

async function saveCell(file, rect, group, name, size=32, trim=true, padding=2) {
  const raw = await sharp(file).extract(rect).ensureAlpha().raw().toBuffer({resolveWithObject:true});
  const w=raw.info.width,h=raw.info.height,d=raw.data;
  let x0=w,y0=h,x1=-1,y1=-1;
  for(let y=0;y<h;y++) for(let x=0;x<w;x++) {
    const a=(y*w+x)*4+3;
    d[a]=d[a]>=160?255:0;
    if(d[a]) {x0=Math.min(x0,x);y0=Math.min(y0,y);x1=Math.max(x1,x);y1=Math.max(y1,y);}
  }
  if(x1<0) throw Error('Empty sprite '+name);
  let p=sharp(d,{raw:{width:w,height:h,channels:4}});
  if(trim) p=p.extract({left:x0,top:y0,width:x1-x0+1,height:y1-y0+1});
  const inner=size-padding*2;
  const buf=await p.resize(inner,inner,{fit:'contain',kernel:'nearest',background:'#00000000'}).extend({top:padding,bottom:padding,left:padding,right:padding,background:'#00000000'}).png({palette:true,colours:32,dither:0}).toBuffer();
  const folder=path.join(dest,group); fs.mkdirSync(folder,{recursive:true});
  const out=path.join(folder,name+'.png');fs.writeFileSync(out,buf);
  manifest.push({group,name,file:path.relative(root,out).replaceAll('\\','/'),width:size,height:size,source:path.relative(root,file).replaceAll('\\','/'),rect});
}
async function atlas(filename,label,xs,ys,names,group) {
  const copy=sourceAtlas(filename,label);
  for(let i=0;i<names.length;i++) {const x=i%(xs.length-1),y=Math.floor(i/(xs.length-1));
    await saveCell(copy,{left:xs[x],top:ys[y],width:xs[x+1]-xs[x],height:ys[y+1]-ys[y]},group,names[i]);}
}
async function main() {
  await atlas('exec-e34286ed-9e50-4bfe-839c-4e0d0c4cd422.png','Items',[0,315,575,830,1090,1402],[0,345,595,815,1122],itemNames,'Items');
  await atlas('exec-2c31bae4-0305-4f5a-befc-d33ff05e71f3.png','Skills',[0,250,500,750,1000,1254],[0,250,500,740,980,1254],skillNames,'Skills');
  await atlas('exec-76987d8e-0519-40a7-888d-81f18ed3ddfd.png','Symbols',[0,350,650,930,1254],[0,350,630,925,1254],['Shop','ShopSign','PlayerMarker','ShopMarker','Lock','Check','Close','Warning','Heart','Oxygen','ArrowDown','ArrowRight','Sparkle','Settings','ScrollHandle','Crafting'],'Symbols');
  const ui=sourceAtlas('exec-332e5f35-060d-46d4-ad2b-499131d10d4b.png','UI');
  const parts=[['Panel',20,65,330,320],['Slot',380,130,230,220],['SelectedSlot',640,130,230,220],['Tooltip',895,110,340,240],['Button',25,425,295,205],['ButtonHover',335,425,300,205],['ButtonPressed',635,425,295,205],['ButtonDisabled',935,425,300,205],['Portrait',10,650,320,300],['Minimap',345,675,255,250],['BarFrame',620,735,300,130],['OxygenFrame',930,735,305,130],['HealthFill',35,1030,300,90],['OxygenFill',350,1030,300,90],['Keycap',670,950,245,245],['SkillNode',960,940,265,280]];
  for(const [name,left,top,width,height]of parts) {
    // UI pieces retain their aspect ratio and are trimmed, ready for nine-slice.
    const crop=await sharp(ui).extract({left,top,width,height}).ensureAlpha().raw().toBuffer({resolveWithObject:true});
    let x0=width,y0=height,x1=0,y1=0;
    for(let y=0;y<height;y++)for(let x=0;x<width;x++){let a=(y*width+x)*4+3;crop.data[a]=crop.data[a]>160?255:0;if(crop.data[a]){x0=Math.min(x0,x);y0=Math.min(y0,y);x1=Math.max(x1,x);y1=Math.max(y1,y);}}
    const out=path.join(dest,'UI',name+'.png');fs.mkdirSync(path.dirname(out),{recursive:true});
    const wide=/Frame|Fill/.test(name);const ow=wide?64:64,oh=wide?20:64;
    await sharp(crop.data,{raw:{width,height,channels:4}}).extract({left:x0,top:y0,width:x1-x0+1,height:y1-y0+1}).resize(ow,oh,{fit:'fill',kernel:'nearest'}).png({palette:true,colours:24,dither:0}).toFile(out);
    manifest.push({group:'UI',name,file:path.relative(root,out).replaceAll('\\','/'),width:ow,height:oh});
  }
  const blocks=path.join(root,'output/imagegen/blocks-and-overlays-v9/blocks.png');
  for(const [i,name] of ['Dirt','Ice','Magma','Rock'].entries()) await saveCell(blocks,{left:i*32,top:0,width:32,height:32},'Terrain',name,32,false,0);
  const damage=path.join(root,'output/imagegen/blocks-and-overlays-v9/damage-overlays.png');
  for(let i=0;i<4;i++)await saveCell(damage,{left:i*32,top:0,width:32,height:32},'Damage','Damage'+(i+1)*20,32,false,0);
  for(const name of itemNames.filter(n=>n.startsWith('Ore_')&&n!=='Ore_Test')) {
    const original=path.join(root,'output/imagegen/mineral-overlays-v8',name+'.png');
    await saveCell(original,{left:0,top:0,width:32,height:32},'Ores',name,32,false,0);
  }
  const terrain=sourceAtlas('exec-65e25b77-650d-4e1e-b0b8-84dd7b7022e0.png','Terrain');
  await saveCell(terrain,{left:630,top:65,width:565,height:560},'Terrain','Sand',32,false,0);
  await saveCell(terrain,{left:60,top:630,width:565,height:565},'Terrain','SupportBeam',32,false,0);
  await saveCell(terrain,{left:630,top:630,width:565,height:565},'Terrain','Cushion',32,false,0);
  await saveCell(terrain,{left:60,top:85,width:565,height:540},'Terrain','Dirt_Grass',32,false,0);
  // Keep approved ground geometry/palette below the new grass cap.
  const grass=await sharp(path.join(dest,'Terrain/Dirt_Grass.png')).extract({left:0,top:0,width:32,height:6}).toBuffer();
  const base=await sharp(path.join(dest,'Terrain/Dirt.png')).composite([{input:grass,left:0,top:0}]).png().toBuffer();
  fs.writeFileSync(path.join(dest,'Terrain/Dirt_Grass.png'),base);
  // Preserve approved source pixels without palette re-quantization.
  for(const [i,name] of ['Dirt','Ice','Magma','Rock'].entries())await sharp(blocks).extract({left:i*32,top:0,width:32,height:32}).toFile(path.join(dest,'Terrain',name+'.png'));
  for(let i=0;i<4;i++)await sharp(damage).extract({left:i*32,top:0,width:32,height:32}).toFile(path.join(dest,'Damage','Damage'+(i+1)*20+'.png'));
  for(const name of itemNames.filter(n=>n.startsWith('Ore_')&&n!=='Ore_Test'))fs.copyFileSync(path.join(root,'output/imagegen/mineral-overlays-v8',name+'.png'),path.join(dest,'Ores',name+'.png'));
  const fx=sourceAtlas('exec-5600d00a-04f4-40c1-98ed-4635199d877f.png','Effects');
  const fxmeta=await sharp(fx).metadata();
  for(const [row,key]of ['BlockBreak','DrillContact','Explosion','Pickup'].entries())for(let col=0;col<4;col++) {
    const x=Math.round(col*fxmeta.width/4),y=Math.round(row*fxmeta.height/4);
    const size={BlockBreak:64,DrillContact:24,Explosion:96,Pickup:20}[key];
    await saveCell(fx,{left:x,top:y,width:Math.round((col+1)*fxmeta.width/4)-x,height:Math.round((row+1)*fxmeta.height/4)-y},'Effects',key+col,size,false,0);
  }
  const bg=sourceAtlas('exec-79b91c00-ead3-42c4-9225-38efbe08fc9a.png','Backgrounds');
  const bgmeta=await sharp(bg).metadata(),half=Math.floor(bgmeta.width/2);
  for(const [i,name]of ['Surface','Underground'].entries())await saveCell(bg,{left:i*half,top:0,width:half,height:bgmeta.height},'Backgrounds',name,name==='Surface'?512:128,false,0);
  fs.writeFileSync(path.join(dest,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
  const preview=path.join(root,'output/art-v1');fs.mkdirSync(preview,{recursive:true});
  for(const group of ['Items','Skills','Symbols','Terrain','UI']) {
    const entries=manifest.filter(e=>e.group===group);const cols=group==='Items'||group==='Skills'?5:4,cell=group==='UI'?144:80;
    const input=await sharp({create:{width:cols*cell,height:Math.ceil(entries.length/cols)*cell,channels:4,background:'#30363d'}}).composite(await Promise.all(entries.map(async(e,i)=>({input:await sharp(path.join(root,e.file)).resize(e.width*2,e.height*2,{kernel:'nearest'}).toBuffer(),left:(i%cols)*cell+8,top:Math.floor(i/cols)*cell+8})))).png().toBuffer().catch(()=>null);
    if(input)fs.writeFileSync(path.join(preview,group+'.png'),input);
  }
  console.log('Prepared '+manifest.length+' assets at '+dest);
}
main().catch(e=>{console.error(e);process.exit(1);});
