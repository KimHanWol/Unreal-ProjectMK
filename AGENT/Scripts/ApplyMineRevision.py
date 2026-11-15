"""Apply the transport, contiguous lumberyard, leaf tile and bounty board revision."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
assets = unreal.AssetToolsHelpers.get_asset_tools()

def import_sprite(name, source, folder):
    task = unreal.AssetImportTask()
    for key,value in dict(filename=str(root/source),destination_path=folder,destination_name='T_'+name,automated=True,replace_existing=True,save=True).items():
        task.set_editor_property(key,value)
    assets.import_asset_tasks([task])
    texture = unreal.load_asset(folder+'/T_'+name)
    for key,value in dict(filter=unreal.TextureFilter.TF_NEAREST,mip_gen_settings=unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,compression_settings=unreal.TextureCompressionSettings.TC_EDITOR_ICON,lod_group=unreal.TextureGroup.TEXTUREGROUP_PIXELS2D).items():
        texture.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(texture,False)
    path=folder+'/S_'+name
    sprite=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else assets.create_asset('S_'+name,folder,unreal.PaperSprite,unreal.PaperSpriteFactory())
    size=unreal.Vector2D(texture.blueprint_get_size_x(),texture.blueprint_get_size_y())
    for key,value in dict(source_texture=texture,source_uv=unreal.Vector2D(0,0),source_dimension=size,source_texture_dimension=size,pixels_per_unreal_unit=2.0,sprite_collision_domain=unreal.SpriteCollisionMode.NONE,default_material=unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial')).items():
        sprite.set_editor_property(key,value)
    geometry=sprite.get_editor_property('render_geometry')
    geometry.set_editor_property('geometry_type',unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
    sprite.set_editor_property('render_geometry',geometry)
    unreal.EditorAssetLibrary.save_loaded_asset(sprite,False)
    return sprite

folder='/Game/Asset/MineRevisionV1'
names=['SupportBeam','Rail','MineCart','AutoMineCart','RailTrestleDown','RailTrestleUp','LumberyardEnclosure','TreeLeaves','BountyBoard','ForestBackdrop']
sprites={name:import_sprite(name,'Content/Asset/MineRevisionV1/Source/'+name+'.png',folder) for name in names}
style=unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
town=dict(style.get_editor_property('town_sprites'))
town.update({name:sprites[name] for name in names if name not in ['TreeLeaves','SupportBeam']})
status_path=folder+'/S_TreeStatus'
status=unreal.load_asset(status_path) if unreal.EditorAssetLibrary.does_asset_exist(status_path) else assets.create_asset('S_TreeStatus',folder,unreal.PaperSprite,unreal.PaperSpriteFactory())
for key,value in dict(source_texture=sprites['LumberyardEnclosure'].get_editor_property('source_texture'),source_uv=unreal.Vector2D(141,10),source_dimension=unreal.Vector2D(8,10),source_texture_dimension=unreal.Vector2D(768,80),pixels_per_unreal_unit=2.0,sprite_collision_domain=unreal.SpriteCollisionMode.NONE,default_material=unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial'),alternate_material=unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial')).items():status.set_editor_property(key,value)
geometry=status.get_editor_property('render_geometry');geometry.set_editor_property('geometry_type',unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX);status.set_editor_property('render_geometry',geometry)
unreal.EditorAssetLibrary.save_loaded_asset(status,False)
town['TreeStatus']=status
town['DrillReach']=unreal.load_asset('/Game/Asset/ArtV1/Skills/S_DrillRange')
style.set_editor_property('town_sprites',town)
unreal.EditorAssetLibrary.save_loaded_asset(style,False)
blocks=unreal.load_asset('/Game/Data/DataTable/BlockDataTable')
rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(blocks))
for row in rows:
    if row['Name'].startswith('TreeLeaves'):row.update(TileSprite=sprites['TreeLeaves'].get_path_name(),bUseTerrainEdges=False,BlockDurability=1,bBreakOnCollapseImpact=True)
    if row['Name']=='TreeTrunk':row.update(BlockDurability=40,bTakesCollapseDamage=False,bDamagesTerrainOnImpact=False)
    if row['Name']=='SupportBeam':row['TileSprite']=sprites['SupportBeam'].get_path_name()
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(blocks,json.dumps(rows,ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(blocks,False)
items=unreal.load_asset('/Game/Data/DataTable/ItemDataTable')
rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(items))
for row in rows:
    if row['Name'] in ['SupportBeam','Rail','MineCart','AutoMineCart']:row['ItemIcon']=sprites[row['Name']].get_path_name()
    if row['Name']=='SupportBeam':row['Description']='쌓아 올려 지형과 철도를 지탱하는 광산 지지대입니다. 윗면에 서서 추가로 설치할 수 있습니다.'
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(items,json.dumps(rows,ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(items,False)
settings=unreal.load_asset('/Game/Data/DataAsset/DA_GameSetting')
f=settings.get_editor_property('forestry')
for key,value in dict(columns=[5,13,21],yard_start_column=0,yard_end_column=35,platform_height=1).items():f.set_editor_property(key,value)
settings.set_editor_property('forestry',f)
t=settings.get_editor_property('town_facilities')
for key,value in dict(village_backdrop_columns=[46,54,62,70,78],forestry_backdrop_columns=[5,19],industrial_backdrop_columns=[96,107]).items():t.set_editor_property(key,value)
quests=list(t.get_editor_property('hunt_quests'))
if quests:
    base=quests[0]
    while len(quests)<4:
        q=unreal.TownQuestDefinition()
        for key in ['target_monster','required_item']:q.set_editor_property(key,base.get_editor_property(key))
        i=len(quests)
        for key,value in dict(title=['갱도 드릴 크롤러 퇴치','채굴 구역 순찰','깊은 갱도 안전 확보','하층 갱도 정리'][i],required_count=[3,5,8,12][i],reward_coins=[75,125,200,300][i],minimum_depth=[0,16,32,48][i]).items():q.set_editor_property(key,value)
        quests.append(q)
t.set_editor_property('hunt_quests',quests)
settings.set_editor_property('town_facilities',t)
unreal.EditorAssetLibrary.save_loaded_asset(settings,False)
# Extend the existing range skills, keeping their original effects and upgrade descriptions.
for path in unreal.EditorAssetLibrary.list_assets('/Game/Data/DataTable',recursive=False):
    if 'Skill' not in path:continue
    table=unreal.load_asset(path)
    if not isinstance(table,unreal.DataTable):continue
    rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    changed=False
    for row in rows:
        if 'DrillRange' in row['Name'] or any('DrillingDistance' in str(effect) for effect in row.get('GameplayEffectList',[])):
            hint='마우스 휠을 올리면 늘어나고 내리면 줄어듭니다. 1칸부터 현재 최대 길이까지 조절할 수 있습니다.'
            description=row.get('Description','')
            if hint not in description:
                if description.startswith('NSLOCTEXT('):
                    namespace,key,original=json.loads('['+description[len('NSLOCTEXT('):-1]+']')
                    row['Description']='NSLOCTEXT('+','.join(json.dumps(v,ensure_ascii=False) for v in [namespace,key,original+'\n\n'+hint])+')'
                else:row['Description']=description+'\n\n'+hint
                changed=True
    if changed:
        assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows,ensure_ascii=False))
        unreal.EditorAssetLibrary.save_loaded_asset(table,False)
actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
removed=[]
for actor in actors.get_all_level_actors():
    if isinstance(actor,unreal.PaperTileMapActor) and 'ArtBackground' not in [str(t) for t in actor.tags]:
        removed.append(actor.get_path_name());actors.destroy_actor(actor)
if removed:unreal.get_editor_subsystem(unreal.LevelEditorSubsystem).save_current_level()
print('Removed unused foreground map actors',removed)
print('Applied mine revision art, four editable hunts, forestry plots, and wheel control descriptions.')
