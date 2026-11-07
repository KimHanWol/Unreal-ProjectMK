// Validate native PNG dimensions, original pixel preservation and data-only visual changes.
const fs = require('fs');
const path = require('path');
const sharp = require('C:/Users/wssh0/.cache/codex-runtimes/codex-primary-runtime/dependencies/node/node_modules/sharp');
const root = path.resolve(__dirname, '../..');
const source = path.join(root, 'Content/Asset/ArtV1/Source');
const readJson = file => JSON.parse(fs.readFileSync(file, 'utf8').replace(/^\uFEFF/, ''));

async function main() {
  const report = {assets:0, originalPixelChecks:[], tableChecks:[], failures:[]};
  const manifest = readJson(path.join(source, 'manifest.json'));
  const v2Manifest=path.join(root,'Content/Asset/ArtV2/Source/manifest.json');
  if(fs.existsSync(v2Manifest))manifest.push(...readJson(v2Manifest));
  for (const entry of manifest) {
    const {data, info} = await sharp(path.join(root, entry.file)).ensureAlpha().raw().toBuffer({resolveWithObject:true});
    if (info.width !== entry.width || info.height !== entry.height) report.failures.push('Dimensions: '+entry.name);
    for (let i=3;i<data.length;i+=4) if (data[i]!==0 && data[i]!==255) {report.failures.push('Soft alpha: '+entry.name);break;}
    report.assets++;
  }
  for (const [i,name] of ['Dirt','Ice','Magma','Rock'].entries()) {
    const original = await sharp(path.join(root,'output/imagegen/blocks-and-overlays-v9/blocks.png')).extract({left:i*32,top:0,width:32,height:32}).ensureAlpha().raw().toBuffer();
    const current = await sharp(path.join(source,'Terrain',name+'.png')).ensureAlpha().raw().toBuffer();
    const equal=original.equals(current);
    report.originalPixelChecks.push({name,equal});
    if(!equal) report.failures.push('Original block changed: '+name);
  }
  for(const entry of manifest.filter(e=>(e.group==='Ores'||e.group==='Damage')&&e.file.includes('/ArtV1/'))) {
    const image=entry.group==='Ores'
      ? sharp(path.join(root,'output/imagegen/mineral-overlays-v8',entry.name+'.png'))
      : sharp(path.join(root,'output/imagegen/blocks-and-overlays-v9/damage-overlays.png')).extract({left:(Number(entry.name.slice(6))/20-1)*32,top:0,width:32,height:32});
    const original=await image.ensureAlpha().raw().toBuffer();
    const current=await sharp(path.join(root,entry.file)).ensureAlpha().raw().toBuffer();
    const equal=original.equals(current);
    report.originalPixelChecks.push({name:entry.name,equal});
    if(!equal)report.failures.push('Original overlay changed: '+entry.name);
  }
  for(const [table,fields] of Object.entries({BlockDataTable:['TileSprite','bUseTerrainEdges','TerrainBlendPriority'],ItemDataTable:['ItemIcon','BlockOverlaySprite'],SkillDataTable:['IconPath']})) {
    const before=readJson(path.join(root,'output/art-v1',table+'-before.json'));
    const v2After=path.join(root,'output/art-v2',table+'-after.json');
    const after=readJson(fs.existsSync(v2After)?v2After:path.join(root,'output/art-v1',table+'-after.json'));
    const clean=rows=>rows.map(row=>Object.fromEntries(Object.entries(row).filter(([key])=>!fields.includes(key))));
    const equal=JSON.stringify(clean(before))===JSON.stringify(clean(after));
    report.tableChecks.push({table,nonVisualFieldsUnchanged:equal,rows:after.length});
    if(!equal)report.failures.push('Gameplay data changed: '+table);
  }
  fs.writeFileSync(path.join(root,'output/art-v1/validation-report.json'),JSON.stringify(report,null,2)+'\n');
  console.log(JSON.stringify(report));
  if(report.failures.length)process.exitCode=1;
}
main().catch(error=>{console.error(error);process.exitCode=1;});
