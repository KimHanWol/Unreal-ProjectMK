"""Read-only ArtV1 asset and optional active PIE validation. Run inside Unreal Editor."""
import json
from pathlib import Path
import unreal

root=Path(unreal.Paths.project_dir())
manifest=json.loads((root/'Content/Asset/ArtV1/Source/manifest.json').read_text(encoding='utf-8-sig'))
for entry in manifest:
    entry['version']='ArtV1'
v2=root/'Content/Asset/ArtV2/Source/manifest.json'
if v2.exists():
    for entry in json.loads(v2.read_text(encoding='utf-8-sig')):
        entry['version']='ArtV2'
        manifest.append(entry)
report={'textures':0,'sprites':0,'flipbooks':0,'runtime':{},'failures':[]}
for entry in manifest:
    stem='/Game/Asset/'+entry['version']+'/'+entry['group']+'/'
    texture=unreal.load_asset(stem+'T_'+entry['name'])
    sprite=unreal.load_asset(stem+'S_'+entry['name'])
    if not texture or not sprite:
        report['failures'].append('Missing: '+stem+entry['name'])
        continue
    report['textures']+=1
    report['sprites']+=1
    dimension=sprite.get_editor_property('source_dimension')
    if dimension.x!=entry['width'] or dimension.y!=entry['height']:
        report['failures'].append('Sprite dimensions: '+entry['name'])
    if sprite.get_editor_property('source_texture')!=texture:
        report['failures'].append('Sprite texture: '+entry['name'])
    if sprite.get_editor_property('pixels_per_unreal_unit')!=entry.get('ppu',2.0):
        report['failures'].append('Sprite pixel density: '+entry['name'])
    if texture.get_editor_property('filter')!=unreal.TextureFilter.TF_NEAREST:
        report['failures'].append('Texture filter: '+entry['name'])
    if texture.get_editor_property('mip_gen_settings')!=unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS:
        report['failures'].append('Texture mipmaps: '+entry['name'])
    if texture.get_editor_property('address_x')!=unreal.TextureAddress.TA_CLAMP or texture.get_editor_property('address_y')!=unreal.TextureAddress.TA_CLAMP:
        report['failures'].append('Texture edge addressing: '+entry['name'])
for name in ['BlockDamage','BlockBreak','DrillContact','Explosion','Pickup']:
    flipbook=unreal.load_asset('/Game/Asset/ArtV1/Effects/FB_'+name)
    if not flipbook or len(flipbook.get_editor_property('key_frames'))!=4:
        report['failures'].append('Flipbook frames: '+name)
    else:
        report['flipbooks']+=1
style=unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
manager=unreal.get_default_object(unreal.load_class(None,'/Game/Core/BP_DataManager.BP_DataManager_C'))
if manager.get_editor_property('visual_style_data_asset')!=style:
    report['failures'].append('DataManager visual style default')
world=unreal.EditorLevelLibrary.get_game_world()
if world:
    runtime_style=unreal.GameplayStatics.get_game_instance(world).get_data_manager().get_editor_property('visual_style_data_asset')
    report['runtime']['styleLoaded']=runtime_style==style
    for name in ['MinimapWidget','PlaytestQuickSlotWidget']:
        widgets=unreal.WidgetLibrary.get_all_widgets_of_class(world,unreal.load_class(None,'/Script/ProjectMK.'+name),False)
        report['runtime'][name]=len(widgets)
    if not report['runtime']['styleLoaded'] or report['runtime']['MinimapWidget']!=1 or report['runtime']['PlaytestQuickSlotWidget']!=8:
        report['failures'].append('Runtime HUD integration')
(root/'output/art-v1/editor-validation-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))
assert not report['failures'],report['failures']
