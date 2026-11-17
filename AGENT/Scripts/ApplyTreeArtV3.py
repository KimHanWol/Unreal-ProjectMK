"""Import the cohesive bark and foliage texture revision."""
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

folder='/Game/Asset/TreeArtV3'
sprites={name:import_sprite(name,'Content/Asset/TreeArtV3/Source/'+name+'.png',folder) for name in ['TreeTrunk','TreeLeaves']}
table=unreal.load_asset('/Game/Data/DataTable/BlockDataTable')
rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
for row in rows:
    if row['Name']=='TreeTrunk':row['TileSprite']=sprites['TreeTrunk'].get_path_name()
    elif row['Name'].startswith('TreeLeaves'):row['TileSprite']=sprites['TreeLeaves'].get_path_name()
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows,ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(table,False)
print('Applied warm bark and olive leaf clusters, preserving terrain-edge masking and tree gameplay data.')
