"""Import town/transport art and register editable factory offers and prototype quest settings."""
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

manifest=json.loads((root/'Content/Asset/LumberyardV1/Source/manifest.json').read_text(encoding='utf-8-sig'))
folder='/Game/Asset/LumberyardV1'
sprites={entry['name']:import_sprite(entry['name'],entry['file'],folder) for entry in manifest['props']}
import_sprite('Canopy','Content/Asset/LumberyardV1/Source/Canopy.png',folder)
texture=unreal.load_asset(folder+'/T_Canopy')
for entry in manifest['leaves']:
    name=entry['name'];path=folder+'/S_'+name
    sprite=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else assets.create_asset('S_'+name,folder,unreal.PaperSprite,unreal.PaperSpriteFactory())
    for key,value in dict(source_texture=texture,source_uv=unreal.Vector2D(entry['x']*32,entry['y']*32),source_dimension=unreal.Vector2D(32,32),source_texture_dimension=unreal.Vector2D(160,96),pixels_per_unreal_unit=2.0,sprite_collision_domain=unreal.SpriteCollisionMode.NONE,default_material=unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial')).items():sprite.set_editor_property(key,value)
    geometry=sprite.get_editor_property('render_geometry');geometry.set_editor_property('geometry_type',unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX);sprite.set_editor_property('render_geometry',geometry)
    unreal.EditorAssetLibrary.save_loaded_asset(sprite,False);sprites[name]=sprite
style=unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
town=dict(style.get_editor_property('town_sprites'));town.update({key:sprites[key] for key in ['Lumberyard','PlantingBed']});style.set_editor_property('town_sprites',town)
decor=dict(style.get_editor_property('mine_decorations'));decor['LanternPost']=sprites['LanternPost'];style.set_editor_property('mine_decorations',decor)
unreal.EditorAssetLibrary.save_loaded_asset(style,False)
blocks=unreal.load_asset('/Game/Data/DataTable/BlockDataTable')
rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(blocks))
legacy=next(row for row in rows if row['Name']=='TreeLeaves');legacy['TileSprite']=sprites['TreeLeaves_2_1'].get_path_name()
next_tile=max(row['TileIndex'] for row in rows)+1
for entry in manifest['leaves']:
    key=entry['name'];row=next((row for row in rows if row['Name']==key),None)
    if row is None:row=dict(legacy,Name=key,TileIndex=next_tile);rows.append(row);next_tile+=1
    row.update(TileSprite=sprites[key].get_path_name(),bUseTerrainEdges=False,BlockDurability=1,bIsMineable=True,bDropOnCollapse=False,bDamagesTerrainOnImpact=False,SpawnableItemDataList=[])
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(blocks,json.dumps(rows,ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(blocks,False)
settings=unreal.load_asset('/Game/Data/DataAsset/DA_GameSetting')
forestry=settings.get_editor_property('forestry')
for key,value in dict(enabled=True,columns=[14,22,30],yard_start_column=3,yard_end_column=35,platform_height=1).items():forestry.set_editor_property(key,value)
settings.set_editor_property('forestry',forestry)
town=settings.get_editor_property('town_facilities')
for key,value in dict(village_backdrop_columns=[7,23,38,49,60,72,83],industrial_backdrop_columns=[96,107],background_parallax_strength=0.18).items():town.set_editor_property(key,value)
settings.set_editor_property('town_facilities',town)
unreal.EditorAssetLibrary.save_loaded_asset(settings,False)
print('Applied contiguous canopy regions, lumberyard shed and planting beds, native larger lamps, and denser continuous parallax layers.')
