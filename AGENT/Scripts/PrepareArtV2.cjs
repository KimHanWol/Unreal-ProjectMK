// Preserve ArtV1; prepare the borderless geology revision from stored generation atlases.
const fs=require('fs');
const path=require('path');
const sharp=require('C:/Users/wssh0/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root=path.resolve(__dirname,'../..');
const source=path.join(root,'Content/Asset/ArtV2/Source');
const manifest=[];
async function save(input,rect,group,name,size,transparent=false) {
  let pipeline=sharp(input).extract(rect).ensureAlpha();
  const {data,info}=await pipeline.raw().toBuffer({resolveWithObject:true});
  for(let i=3;i<data.length;i+=4)data[i]=data[i]>=160?255:0;
  pipeline=sharp(data,{raw:{width:info.width,height:info.height,channels:4}});
  if(transparent)pipeline=pipeline.trim({background:'#00000000',threshold:0}).resize(size-4,size-4,{fit:'contain',kernel:'nearest',background:'#00000000'}).extend({left:2,right:2,top:2,bottom:2,background:'#00000000'});
  else pipeline=pipeline.resize(size,size,{fit:'fill',kernel:'nearest'});
  const out=path.join(source,group,name+'.png');fs.mkdirSync(path.dirname(out),{recursive:true});
  await pipeline.png({palette:true,colours:group==='Backgrounds'?16:8,dither:0}).toFile(out);
  manifest.push({group,name,file:path.relative(root,out).replaceAll('\\','/'),width:size,height:size,ppu:2,source:path.relative(root,input).replaceAll('\\','/'),rect});
}
async function main(){
  const terrain=path.join(source,'Atlases/Terrain.png');const tm=await sharp(terrain).metadata();
  for(const [i,name]of ['Dirt','Rock','Sand','Ice','Magma','Dirt_Grass'].entries()){
    const x=i%3,y=Math.floor(i/3),left=Math.round(x*tm.width/3),top=Math.round(y*tm.height/2);
    await save(terrain,{left,top,width:Math.round((x+1)*tm.width/3)-left,height:Math.round((y+1)*tm.height/2)-top},'Terrain',name,32);
  }
  const background=path.join(source,'Atlases/Underground.png');const bm=await sharp(background).metadata();
  await save(background,{left:0,top:0,width:bm.width,height:bm.height},'Backgrounds','Underground',512);
  const ores=path.join(source,'Atlases/Ores.png');const om=await sharp(ores).metadata();
  for(const [i,name]of ['Coal','Copper','Iron','Gold','Emerald','Diamond','Titanium','Lithium'].entries()){
    const x=i%4,y=Math.floor(i/4),left=Math.round(x*om.width/4),top=Math.round(y*om.height/2);
    await save(ores,{left,top,width:Math.round((x+1)*om.width/4)-left,height:Math.round((y+1)*om.height/2)-top},'Ores','Ore_'+name,32,true);
  }
  fs.writeFileSync(path.join(source,'manifest.json'),JSON.stringify(manifest,null,2)+'\n');
  const preview=path.join(root,'output/art-v2');fs.mkdirSync(preview,{recursive:true});
  const entries=manifest.filter(e=>e.group==='Terrain');
  await sharp({create:{width:384,height:256,channels:4,background:'#30363d'}}).composite(await Promise.all(entries.map(async(e,i)=>({input:await sharp(path.join(root,e.file)).resize(128,128,{kernel:'nearest'}).toBuffer(),left:i%3*128,top:Math.floor(i/3)*128})))).png().toFile(path.join(preview,'terrain.png'));
  const oreEntries=manifest.filter(e=>e.group==='Ores');
  const composite=[];
  for(let row=0;row<2;row++)for(let i=0;i<8;i++){
    const base=path.join(source,'Terrain',row===0?'Dirt.png':'Rock.png');
    const tile=await sharp(base).composite([{input:path.join(root,oreEntries[i].file)}]).png().toBuffer();
    composite.push({input:await sharp(tile).resize(96,96,{kernel:'nearest'}).toBuffer(),left:i*96,top:row*96});
  }
  await sharp({create:{width:768,height:192,channels:4,background:'#30363d'}}).composite(composite).png().toFile(path.join(preview,'ore-composite.png'));
  console.log('Prepared '+manifest.length+' ArtV2 assets.');
}
main().catch(error=>{console.error(error);process.exitCode=1;});
