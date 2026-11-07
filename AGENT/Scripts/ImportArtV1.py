"""Import prepared PNGs as pixel textures and collision-free Paper2D sprites."""
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
VERSION = globals().get('ART_VERSION', 'ArtV1')
BASE = '/Game/Asset/' + VERSION
manifest = json.loads((ROOT / 'Content/Asset' / VERSION / 'Source/manifest.json').read_text(encoding='utf-8-sig'))
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
material = unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial')
results = []
for entry in manifest:
    folder = BASE + '/' + entry['group']
    name = entry['name']
    texture_path = folder + '/T_' + name
    texture = unreal.load_asset(texture_path) if unreal.EditorAssetLibrary.does_asset_exist(texture_path) else None
    if not texture:
        task = unreal.AssetImportTask()
        task.set_editor_property('filename', str(ROOT / entry['file']))
        task.set_editor_property('destination_path', folder)
        task.set_editor_property('destination_name', 'T_' + name)
        task.set_editor_property('automated', True)
        task.set_editor_property('save', False)
        task.set_editor_property('replace_existing', False)
        asset_tools.import_asset_tasks([task])
        texture = unreal.load_asset(texture_path)
    if not texture:
        raise RuntimeError('Texture import failed: ' + name)
    texture.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI if entry['group'] in ['UI','Skills','Symbols'] else unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    sprite_path = folder + '/S_' + name
    sprite = unreal.load_asset(sprite_path) if unreal.EditorAssetLibrary.does_asset_exist(sprite_path) else asset_tools.create_asset('S_' + name, folder, unreal.PaperSprite, unreal.PaperSpriteFactory())
    sprite.set_editor_property('source_texture', texture)
    sprite.set_editor_property('source_uv', unreal.Vector2D(0,0))
    sprite.set_editor_property('source_dimension', unreal.Vector2D(entry['width'],entry['height']))
    sprite.set_editor_property('source_texture_dimension', unreal.Vector2D(entry['width'],entry['height']))
    sprite.set_editor_property('pixels_per_unreal_unit', 2.0)
    sprite.set_editor_property('pivot_mode', unreal.SpritePivotMode.CENTER_CENTER)
    sprite.set_editor_property('sprite_collision_domain', unreal.SpriteCollisionMode.NONE)
    sprite.set_editor_property('default_material', material)
    geometry = sprite.get_editor_property('render_geometry')
    geometry.set_editor_property('geometry_type', unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
    sprite.set_editor_property('render_geometry', geometry)
    unreal.EditorAssetLibrary.save_loaded_asset(sprite)
    results.append({'name':name,'group':entry['group'],'texture':texture.get_path_name(),'sprite':sprite.get_path_name()})

report = ROOT / ('output/art-v2' if VERSION == 'ArtV2' else 'output/art-v1') / 'import-report.json'
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps(results,ensure_ascii=False,indent=2),encoding='utf-8')
for table_name in ['BlockDataTable','ItemDataTable','SkillDataTable']:
    table = unreal.load_asset('/Game/Data/DataTable/' + table_name)
    before = report.parent / (table_name + '-before.json')
    if not before.exists():
        before.write_text(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table),encoding='utf-8')
print('Imported and saved',len(results),'textures and sprites.')
