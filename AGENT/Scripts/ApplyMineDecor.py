"""Import native-size, non-colliding mine decorations and register their shared visual roles."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
manifest = json.loads((root / 'Content/Asset/MineDecorV1/Source/manifest.json').read_text(encoding='utf-8-sig'))
tools = unreal.AssetToolsHelpers.get_asset_tools()
folder = '/Game/Asset/MineDecorV1'
style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
decorations = dict(style.get_editor_property('mine_decorations'))
for entry in manifest:
    task = unreal.AssetImportTask()
    for name, value in dict(filename=str(root / entry['file']), destination_path=folder,
        destination_name='T_' + entry['name'], automated=True, replace_existing=True, save=True).items():
        task.set_editor_property(name, value)
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(folder + '/T_' + entry['name'])
    texture.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
    assert unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
    path = folder + '/S_' + entry['name']
    sprite = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset('S_' + entry['name'], folder, unreal.PaperSprite, unreal.PaperSpriteFactory())
    for name, value in dict(source_texture=texture, source_uv=unreal.Vector2D(0, 0),
        source_dimension=unreal.Vector2D(entry['width'], entry['height']),
        source_texture_dimension=unreal.Vector2D(entry['width'], entry['height']), pixels_per_unreal_unit=2.0,
        pivot_mode=unreal.SpritePivotMode.CENTER_CENTER, sprite_collision_domain=unreal.SpriteCollisionMode.NONE,
        default_material=unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial')).items():
        sprite.set_editor_property(name, value)
    geometry = sprite.get_editor_property('render_geometry')
    geometry.set_editor_property('geometry_type', unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
    sprite.set_editor_property('render_geometry', geometry)
    assert unreal.EditorAssetLibrary.save_loaded_asset(sprite, False)
    decorations[entry['name']] = sprite
style.set_editor_property('mine_decorations', decorations)
assert unreal.EditorAssetLibrary.save_loaded_asset(style, False)
print('Registered six mine decorations at 2 pixels per Unreal unit')
