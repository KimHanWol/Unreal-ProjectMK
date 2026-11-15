"""Import separate notices and the forestry polish assets."""
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

folder='/Game/Asset/ForestryBoardV2'
names=['BountyBoard','BountyPaper','LumberyardEnclosure','TreeStatusGreen','TreeStatusRed','SupportBeam','Lumberyard']
sprites={name:import_sprite(name,'Content/Asset/ForestryBoardV2/Source/'+name+'.png',folder) for name in names}
style=unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
town=dict(style.get_editor_property('town_sprites'))
town.update({name:sprite for name,sprite in sprites.items() if name!='SupportBeam'})
town.pop('TreeStatus',None)
style.set_editor_property('town_sprites',town)
unreal.EditorAssetLibrary.save_loaded_asset(style,False)
blocks=unreal.load_asset('/Game/Data/DataTable/BlockDataTable')
rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(blocks))
for row in rows:
    if row['Name'].startswith('TreeLeaves'):
        row.update(bUseTerrainEdges=True,TerrainBlendPriority=12,BlockDurability=2,bBreakOnCollapseImpact=True,bDamagesTerrainOnImpact=False)
    if row['Name']=='TreeTrunk':
        row.update(BlockDurability=140,bUseTerrainEdges=True,TerrainBlendPriority=12,bTakesCollapseDamage=False,bDamagesTerrainOnImpact=False)
    if row['Name']=='SupportBeam':row['TileSprite']=sprites['SupportBeam'].get_path_name()
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(blocks,json.dumps(rows,ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(blocks,False)
items=unreal.load_asset('/Game/Data/DataTable/ItemDataTable')
rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(items))
for row in rows:
    if row['Name']=='SupportBeam':row['ItemIcon']=sprites['SupportBeam'].get_path_name()
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(items,json.dumps(rows,ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(items,False)
settings=unreal.load_asset('/Game/Data/DataAsset/DA_GameSetting')
f=settings.get_editor_property('forestry')
f.set_editor_property('growth_stage_seconds',25)
settings.set_editor_property('forestry',f)
t=settings.get_editor_property('town_facilities')
for key,value in dict(industrial_backdrop_columns=[100,110],board_roll_interval_seconds=30,board_offer_chance=0.35,board_notice_min_seconds=120,board_notice_max_seconds=300).items():t.set_editor_property(key,value)
settings.set_editor_property('town_facilities',t)
unreal.EditorAssetLibrary.save_loaded_asset(settings,False)
print('Applied independent notice art, low enclosure, two lamp states, 140 HP trunks, shared terrain edges and staged growth settings.')
