"""Import the generated role atlas without changing gameplay tables or existing UI art."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
manifest = json.loads((root / 'Content/Asset/WidgetArtV2/Source/manifest.json').read_text(encoding='utf-8-sig'))
style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
brushes = {str(key): value for key, value in style.get_editor_property('brushes').items()}
tools = unreal.AssetToolsHelpers.get_asset_tools()
base = '/Game/Asset/WidgetArtV2/UI'
for entry in manifest:
    task = unreal.AssetImportTask()
    task.set_editor_property('filename', str(root / entry['file']))
    task.set_editor_property('destination_path', base)
    task.set_editor_property('destination_name', 'T_' + entry['name'])
    task.set_editor_property('automated', True)
    task.set_editor_property('replace_existing', True)
    task.set_editor_property('save', True)
    tools.import_asset_tasks([task])
    texture = unreal.load_asset(base + '/T_' + entry['name'])
    if texture is None:
        raise RuntimeError('Import failed: ' + entry['name'])
    texture.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
    texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
    texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
    texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
    texture.set_editor_property('address_x', unreal.TextureAddress.TA_CLAMP)
    texture.set_editor_property('address_y', unreal.TextureAddress.TA_CLAMP)
    if not unreal.EditorAssetLibrary.save_loaded_asset(texture, False):
        raise RuntimeError('Texture save failed: ' + entry['name'])
    brush = unreal.WidgetLibrary.make_brush_from_texture(texture, entry['width'], entry['height'])
    box = entry['name'] != 'SectionDivider' and not entry['name'].startswith('Icon')
    brush.set_editor_property('draw_as', unreal.SlateBrushDrawType.BOX if box else unreal.SlateBrushDrawType.IMAGE)
    if box:
        brush.set_editor_property('margin', unreal.Margin(entry['marginX']/entry['width'], entry['marginY']/entry['height'], entry['marginX']/entry['width'], entry['marginY']/entry['height']))
    brushes[entry['name']] = brush
style.set_editor_property('brushes', brushes)
if not unreal.EditorAssetLibrary.save_loaded_asset(style, False):
    raise RuntimeError('Shared style save failed')
print('Imported sixteen UI role textures and updated shared style.')
