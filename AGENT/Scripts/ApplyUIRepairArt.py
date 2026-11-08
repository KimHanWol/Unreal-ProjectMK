"""Apply native-size nine-slice brushes, world shop art, and authored HUD alignment."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
brushes = {str(key): value for key, value in style.get_editor_property('brushes').items()}
manifest = json.loads((root / 'Content/Asset/WidgetArtV1/Source/manifest.json').read_text(encoding='utf-8-sig'))
for entry in manifest:
    texture = unreal.load_asset('/Game/Asset/WidgetArtV1/UI/T_' + entry['name'])
    brush = unreal.WidgetLibrary.make_brush_from_texture(texture, entry['width'], entry['height'])
    brush.set_editor_property('draw_as', unreal.SlateBrushDrawType.BOX)
    brush.set_editor_property('margin', unreal.Margin(entry['marginX']/entry['width'], entry['marginY']/entry['height'], entry['marginX']/entry['width'], entry['marginY']/entry['height']))
    brushes[entry['name']] = brush
rank = brushes['InsetPanel'].copy()
rank.set_editor_property('tint_color', unreal.SlateColor(specified_color=unreal.LinearColor(1.0, 0.84, 0.55, 1.0)))
brushes['RankBadge'] = rank
acquired = brushes['SelectedSlot'].copy()
acquired.set_editor_property('tint_color', unreal.SlateColor(specified_color=unreal.LinearColor(0.35, 1.0, 0.72, 1.0)))
brushes['AcquiredSlot'] = acquired
portrait = unreal.load_object(None, '/Game/UI/BP/W_CharacterInfo.W_CharacterInfo:WidgetTree.Image_83')
brushes['PlayerPortrait'] = portrait.get_editor_property('brush')
scroll = unreal.WidgetLibrary.make_brush_from_texture(unreal.load_asset('/Game/Asset/ArtV1/Symbols/T_ScrollHandle'), 12, 32)
scroll.set_editor_property('draw_as', unreal.SlateBrushDrawType.BOX)
scroll.set_editor_property('margin', unreal.Margin(0.25, 0.25, 0.25, 0.25))
brushes['ScrollHandle'] = scroll

folder = '/Game/Asset/WorldArtV3'
tools = unreal.AssetToolsHelpers.get_asset_tools()
task = unreal.AssetImportTask()
task.set_editor_property('filename', str(root / 'Content/Asset/WorldArtV3/Source/Shop.png'))
task.set_editor_property('destination_path', folder)
task.set_editor_property('destination_name', 'T_Shop')
task.set_editor_property('automated', True)
task.set_editor_property('replace_existing', True)
task.set_editor_property('save', True)
tools.import_asset_tasks([task])
texture = unreal.load_asset(folder + '/T_Shop')
texture.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
unreal.EditorAssetLibrary.save_loaded_asset(texture)
sprite = unreal.load_asset(folder + '/S_Shop') if unreal.EditorAssetLibrary.does_asset_exist(folder + '/S_Shop') else tools.create_asset('S_Shop', folder, unreal.PaperSprite, unreal.PaperSpriteFactory())
sprite.set_editor_property('source_texture', texture)
sprite.set_editor_property('source_uv', unreal.Vector2D(0, 0))
sprite.set_editor_property('source_dimension', unreal.Vector2D(128, 96))
sprite.set_editor_property('source_texture_dimension', unreal.Vector2D(128, 96))
sprite.set_editor_property('pixels_per_unreal_unit', 2.0)
sprite.set_editor_property('pivot_mode', unreal.SpritePivotMode.CENTER_CENTER)
sprite.set_editor_property('sprite_collision_domain', unreal.SpriteCollisionMode.NONE)
sprite.set_editor_property('default_material', unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial'))
geometry = sprite.get_editor_property('render_geometry')
geometry.set_editor_property('geometry_type', unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
sprite.set_editor_property('render_geometry', geometry)
unreal.EditorAssetLibrary.save_loaded_asset(sprite)
table = unreal.load_asset('/Game/Data/DataTable/BlockDataTable')
rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
for row in rows:
    if row['Name'] == 'Shop':
        row['TileSprite'] = sprite.get_path_name()
unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows))
unreal.EditorAssetLibrary.save_loaded_asset(table)
shop_brush = unreal.WidgetLibrary.make_brush_from_texture(texture, 128, 96)
shop_brush.set_editor_property('draw_as', unreal.SlateBrushDrawType.IMAGE)
brushes['Shop'] = shop_brush
style.set_editor_property('brushes', brushes)
unreal.EditorAssetLibrary.save_loaded_asset(style)

def widget(asset, name):
    return unreal.load_object(None, asset + '.' + asset.rsplit('/',1)[-1] + ':WidgetTree.' + name)

character = '/Game/UI/BP/W_CharacterInfo'
bars = widget(character, 'VerticalBox_133')
slot = bars.get_editor_property('slot')
layout = slot.get_editor_property('layout_data')
layout.offsets = unreal.Margin(168, 92, 260, 64)
slot.set_editor_property('layout_data', layout)
background = widget(character, 'Image_138')
layout = background.get_editor_property('slot').get_editor_property('layout_data')
layout.offsets = unreal.Margin(0, 0, 444, 168)
background.get_editor_property('slot').set_editor_property('layout_data', layout)
portrait_area = widget(character, 'Overlay_48')
layout = portrait_area.get_editor_property('slot').get_editor_property('layout_data')
layout.offsets = unreal.Margin(12, 12, 144, 144)
portrait_area.get_editor_property('slot').set_editor_property('layout_data', layout)
for name in ['Image_63','Image_83']:
    portrait_slot = widget(character, name).get_editor_property('slot')
    portrait_slot.set_editor_property('horizontal_alignment', unreal.HorizontalAlignment.H_ALIGN_FILL)
    portrait_slot.set_editor_property('vertical_alignment', unreal.VerticalAlignment.V_ALIGN_FILL)

hud = '/Game/UI/BP/W_HUD'
for name in ['CharacterInfo','CurrencyInfo','Inventory']:
    item = widget(hud, name)
    slot = item.get_editor_property('slot')
    layout = slot.get_editor_property('layout_data')
    if name == 'CharacterInfo':
        layout.offsets = unreal.Margin(12, -12, 444, 168)
        layout.anchors = unreal.Anchors(unreal.Vector2D(0,1), unreal.Vector2D(0,1))
        layout.alignment = unreal.Vector2D(0,1)
    elif name == 'CurrencyInfo':
        item.set_render_translation(unreal.Vector2D(0,0))
        layout.offsets = unreal.Margin(-12, 12, 132, 48)
        layout.anchors = unreal.Anchors(unreal.Vector2D(1,0), unreal.Vector2D(1,0))
        layout.alignment = unreal.Vector2D(1,0)
        slot.set_editor_property('auto_size', True)
    else:
        layout.offsets = unreal.Margin(-12, -12, 312, 256)
        layout.anchors = unreal.Anchors(unreal.Vector2D(1,1), unreal.Vector2D(1,1))
        layout.alignment = unreal.Vector2D(1,1)
        slot.set_editor_property('auto_size', True)
    slot.set_editor_property('layout_data', layout)
for asset in [character, hud]:
    blueprint = unreal.load_asset(asset)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
print('Applied native brush sizes, shared state badges, scrollbar, 128x96 shop, HUD alignment.')
exec((root / 'AGENT/Scripts/ApplyInventoryUX.py').read_text(encoding='utf-8-sig'), {})
