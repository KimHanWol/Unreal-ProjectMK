// Compare actual GPU-rendered material masks, including the protected center.
const fs=require('fs');
const path=require('path');
const sharp=require('C:/Users/wssh0/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root=path.resolve(__dirname,'../..');
const output=path.join(root,'output/terrain-edges');
async function main(){
  const images=[];
  for(let mask=0;mask<16;mask++)images.push(await sharp(path.join(output,'gpu',`edges-${String(mask).padStart(2,'0')}.png`)).ensureAlpha().raw().toBuffer());
  const failures=[];
  const changed=[];
  for(let mask=0;mask<16;mask++){
    let centerDifferences=0,edgeDifferences=0;
    for(let y=0;y<32;y++)for(let x=0;x<32;x++){
      const i=(y*32+x)*4;
      if(!images[0].subarray(i,i+4).equals(images[mask].subarray(i,i+4))){
        if(x>=4&&x<28&&y>=4&&y<28)centerDifferences++;
        else edgeDifferences++;
      }
    }
    if(centerDifferences)failures.push(`Mask ${mask}: ${centerDifferences} central pixels changed`);
    if(mask>0&&!edgeDifferences)failures.push(`Mask ${mask}: no edge change`);
    changed.push({mask,centerDifferences,edgeDifferences});
  }
  // A single exposed left face must leave the connected right face intact.
  for(let y=0;y<32;y++){
    const i=(y*32+31)*4;
    if(!images[0].subarray(i,i+4).equals(images[1].subarray(i,i+4)))failures.push(`Connected right edge changed at ${y}`);
  }
  const unique=new Set(images.map(buffer=>buffer.toString('base64'))).size;
  if(unique!==16)failures.push(`Only ${unique} distinct masks`);
  // Read the rendered silhouette: short teeth stay in-band without full-height square jumps.
  const profiles=[];
  for(const mask of [1,2,4,8]){
    const depths=[];
    for(let along=0;along<32;along++){
      let depth=0;
      for(;depth<32;depth++){
        const x=mask===1?depth:mask===2?31-depth:along;
        const y=mask===4?depth:mask===8?31-depth:along;
        const i=(y*32+x)*4;
        if(images[mask][i]+images[mask][i+1]+images[mask][i+2]>0)break;
      }
      depths.push(depth);
    }
    const maxStep=Math.max(...depths.slice(1).map((depth,i)=>Math.abs(depth-depths[i])));
    if(maxStep>2||depths.some(depth=>depth<1||depth>4))failures.push(`Mask ${mask}: abrupt or out-of-band silhouette`);
    profiles.push({mask,depths,maxStep});
  }
  const transitions=[];
  for(const mask of [1,2,4,8,15]){
    const data=await sharp(path.join(output,'gpu',`filled-${String(mask).padStart(2,'0')}.png`)).ensureAlpha().raw().toBuffer();
    let holes=0,centerDifferences=0;
    for(let y=0;y<32;y++)for(let x=0;x<32;x++){
      const i=(y*32+x)*4;
      if(data[i]+data[i+1]+data[i+2]===0)holes++;
      if(x>=4&&x<28&&y>=4&&y<28&&!images[0].subarray(i,i+3).equals(data.subarray(i,i+3)))centerDifferences++;
    }
    transitions.push({mask,holes,centerDifferences});
    if(holes||centerDifferences)failures.push(`Transition ${mask}: holes=${holes}, central changes=${centerDifferences}`);
  }
  const report={renderedMasks:16,protectedCenter:'24x24',changed,profiles,transitions,failures};
  fs.writeFileSync(path.join(output,'pixel-validation.json'),JSON.stringify(report,null,2)+'\n');
  // Canvas exports retain RGB but may leave alpha at zero; make only the diagnostic sheet opaque.
  const previews=images.map(data=>{const copy=Buffer.from(data);for(let i=3;i<copy.length;i+=4)copy[i]=255;return copy;});
  await sharp({create:{width:544,height:544,channels:4,background:'#68656a'}}).composite(await Promise.all(previews.map(async(data,i)=>({input:await sharp(data,{raw:{width:32,height:32,channels:4}}).resize(128,128,{kernel:'nearest'}).png().toBuffer(),left:i%4*136+4,top:Math.floor(i/4)*136+4})))).png().toFile(path.join(output,'mask-preview.png'));
  console.log(JSON.stringify(report));
  if(failures.length)process.exitCode=1;
}
main().catch(error=>{console.error(error);process.exitCode=1;});
