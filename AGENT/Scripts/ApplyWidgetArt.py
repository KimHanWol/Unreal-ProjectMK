"""Apply the widget skin to shared style roles and existing Widget Blueprint surfaces."""
import json
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
BASE = '/Game/Asset/WidgetArtV1/UI'
manifest = json.loads((ROOT / 'Content/Asset/WidgetArtV1/Source/manifest.json').read_text(encoding='utf-8-sig'))
style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
if not style:
    raise RuntimeError('Missing DA_VisualStyle')
brushes = dict(style.get_editor_property('brushes'))
for entry in manifest:
    texture = unreal.load_asset(BASE + '/T_' + entry['name'])
    if not texture:
        raise RuntimeError('Missing widget texture: ' + entry['name'])
    brush = unreal.SlateBrush()
    brush.set_editor_property('resource_object', texture)
    brush.set_editor_property('draw_as', unreal.SlateBrushDrawType.BOX)
    brush.set_editor_property('margin', unreal.Margin(
        entry['marginX'] / entry['width'], entry['marginY'] / entry['height'],
        entry['marginX'] / entry['width'], entry['marginY'] / entry['height']))
    brushes[entry['name']] = brush
currency_texture = unreal.load_asset('/Game/Asset/ArtV1/Items/T_Coin')
if not currency_texture:
    raise RuntimeError('Missing currency texture')
currency_brush = unreal.SlateBrush()
currency_brush.set_editor_property('resource_object', currency_texture)
currency_brush.set_editor_property('draw_as', unreal.SlateBrushDrawType.IMAGE)
brushes['Currency'] = currency_brush
style.set_editor_property('brushes', brushes)
unreal.EditorAssetLibrary.save_loaded_asset(style)

blueprint_images = {
    '/Game/UI/BP/Menu/W_MenuBase': {'BG':'Panel'},
    '/Game/UI/BP/Inventory/W_Inventory': {'Image_53':'InsetPanel'},
    '/Game/UI/BP/Slot/W_ItemSlot': {'Slot_BG':'Slot','FocusImage':'SelectedSlot'},
    '/Game/UI/BP/Slot/W_SkillSlot': {'BG':'Slot','FocusImage':'SelectedSlot'},
    '/Game/UI/BP/Tooltip/W_Tooltip': {'BG':'Tooltip'},
    '/Game/UI/BP/W_HealthBar': {'Image_102':'BarFrame'},
    '/Game/UI/BP/W_OxygenBar': {'Image_102':'OxygenFrame'},
    '/Game/UI/BP/Menu/SkillTree/W_SkillTreeMenuContents': {'BG':'InsetPanel'},
}
applied = []
for blueprint_path, image_roles in blueprint_images.items():
    blueprint = unreal.load_asset(blueprint_path)
    if not blueprint:
        raise RuntimeError('Missing widget blueprint: ' + blueprint_path)
    asset_name = blueprint_path.rsplit('/', 1)[-1]
    for image_name, role in image_roles.items():
        image = unreal.load_object(None, blueprint_path + '.' + asset_name + ':WidgetTree.' + image_name)
        if not image:
            raise RuntimeError('Missing image ' + image_name + ' in ' + blueprint_path)
        image.set_editor_property('brush', brushes[role])
        applied.append(blueprint_path + ':' + image_name + '=' + role)
    unreal.BlueprintEditorLibrary.compile_blueprint(blueprint)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)

report = {'brushes': sorted(entry['name'] for entry in manifest), 'blueprintImages': applied}
(ROOT / 'output/widget-art/application-report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(report, ensure_ascii=False))
