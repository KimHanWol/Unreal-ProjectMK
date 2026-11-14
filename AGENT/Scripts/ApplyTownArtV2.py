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

manifest=json.loads((root/'Content/Asset/TownArtV2/Source/manifest.json').read_text(encoding='utf-8-sig'))
style=unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
sprites=dict(style.get_editor_property('town_sprites'))
for entry in manifest:
    sprites[entry['name']]=import_sprite(entry['name'],entry['file'],'/Game/Asset/TownArtV2')
style.set_editor_property('town_sprites',sprites)
unreal.EditorAssetLibrary.save_loaded_asset(style,False)
settings=unreal.load_asset('/Game/Data/DataAsset/DA_GameSetting')
town=settings.get_editor_property('town_facilities')
for field,value in dict(house_a_column=54,house_b_column=62,rail_factory_column=108,village_backdrop_columns=[49,61],village_notice_column=66).items():
    town.set_editor_property(field,value)
settings.set_editor_property('town_facilities',town)
unreal.EditorAssetLibrary.save_loaded_asset(settings,False)
print('Applied larger native facility sprites, clustered houses and village backdrop settings. Storage chest and transport sprites are unchanged.')
