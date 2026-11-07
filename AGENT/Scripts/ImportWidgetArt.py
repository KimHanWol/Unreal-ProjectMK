"""Import the role-specific widget skin textures with crisp UI rendering settings."""
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
BASE = '/Game/Asset/WidgetArtV1/UI'
manifest = json.loads((ROOT / 'Content/Asset/WidgetArtV1/Source/manifest.json').read_text(encoding='utf-8-sig'))
asset_tools = unreal.AssetToolsHelpers.get_asset_tools()
results = []
for entry in manifest:
    texture_path = BASE + '/T_' + entry['name']
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(ROOT / entry['file']))
    task.set_editor_property('destination_path', BASE)
    task.set_editor_property('destination_name', 'T_' + entry['name'])
    task.set_editor_property('automated', True)
    task.set_editor_property('save', False)
    task.set_editor_property('replace_existing', True)
    asset_tools.import_asset_tasks([task])
    texture = unreal.load_asset(texture_path)
    if not texture:
        raise RuntimeError('Texture import failed: ' + entry['name'])
    texture.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('srgb', True)
    texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    unreal.EditorAssetLibrary.save_loaded_asset(texture)
    results.append({'name':entry['name'],'texture':texture.get_path_name()})

report = ROOT / 'output/widget-art/import-report.json'
report.parent.mkdir(parents=True, exist_ok=True)
report.write_text(json.dumps(results, ensure_ascii=False, indent=2), encoding='utf-8')
print('Imported and saved', len(results), 'widget textures.')
