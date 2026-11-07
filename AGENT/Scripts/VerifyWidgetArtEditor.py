"""Verify imported widget textures, shared brush roles, saved Blueprint references, and live widgets."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
manifest = json.loads((root / 'Content/Asset/WidgetArtV1/Source/manifest.json').read_text(encoding='utf-8-sig'))
style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
brushes = {str(name):brush for name, brush in style.get_editor_property('brushes').items()} if style else {}
failures = []
for entry in manifest:
    name = entry['name']
    texture = unreal.load_asset('/Game/Asset/WidgetArtV1/UI/T_' + name)
    if not texture:
        failures.append('Missing texture ' + name)
        continue
    if texture.get_editor_property('filter') != unreal.TextureFilter.TF_NEAREST:
        failures.append('Non-nearest texture ' + name)
    brush = brushes.get(name)
    resource = brush.get_editor_property('resource_object') if brush else None
    if not resource or '/Game/Asset/WidgetArtV1/' not in resource.get_path_name():
        failures.append('Invalid brush ' + name)

currency_brush = brushes.get('Currency')
currency_resource = currency_brush.get_editor_property('resource_object') if currency_brush else None
if not currency_resource or currency_resource.get_path_name() != '/Game/Asset/ArtV1/Items/T_Coin.T_Coin':
    failures.append('Invalid currency brush')

blueprint_images = {
    '/Game/UI/BP/Menu/W_MenuBase.W_MenuBase:WidgetTree.BG':'Panel',
    '/Game/UI/BP/Inventory/W_Inventory.W_Inventory:WidgetTree.Image_53':'InsetPanel',
    '/Game/UI/BP/Slot/W_ItemSlot.W_ItemSlot:WidgetTree.Slot_BG':'Slot',
    '/Game/UI/BP/Slot/W_ItemSlot.W_ItemSlot:WidgetTree.FocusImage':'SelectedSlot',
    '/Game/UI/BP/Tooltip/W_Tooltip.W_Tooltip:WidgetTree.BG':'Tooltip',
    '/Game/UI/BP/W_HealthBar.W_HealthBar:WidgetTree.Image_102':'BarFrame',
    '/Game/UI/BP/W_OxygenBar.W_OxygenBar:WidgetTree.Image_102':'OxygenFrame',
}
for object_path, role in blueprint_images.items():
    image = unreal.load_object(None, object_path)
    resource = image.get_editor_property('brush').get_editor_property('resource_object') if image else None
    if not resource or resource.get_name() != 'T_' + role:
        failures.append('Blueprint brush mismatch ' + object_path)

layout = json.loads((root / 'output/widget-art/layout-v2-application.json').read_text(encoding='utf-8-sig'))
if layout['characterStatus']['gap'] < 8.0:
    failures.append('Character portrait and status bars overlap')
currency_text = unreal.load_object(None, '/Game/UI/BP/W_Currency.W_Currency:WidgetTree.Text_Coin')
currency_padding = currency_text.get_editor_property('slot').get_editor_property('padding') if currency_text else None
if not currency_padding or currency_padding.left < 4.0:
    failures.append('Currency icon and amount have insufficient spacing')

runtime = {'quickSlots':0, 'recipeCards':0}
world = unreal.EditorLevelLibrary.get_game_world()
if world:
    quick_class = unreal.load_class(None, '/Script/ProjectMK.PlaytestQuickSlotWidget')
    recipe_class = unreal.load_class(None, '/Script/ProjectMK.CraftingRecipeSlotWidget')
    quick_slots = unreal.WidgetLibrary.get_all_widgets_of_class(world, quick_class, False)
    recipes = unreal.WidgetLibrary.get_all_widgets_of_class(world, recipe_class, False)
    runtime['quickSlots'] = len(quick_slots)
    runtime['recipeCards'] = len(recipes)
    if len(quick_slots) != 8:
        failures.append('Quick slot layout mismatch')
    if not recipes:
        failures.append('No runtime recipe cards')

report = {
    'textures': len(manifest),
    'brushes': len([name for name in [entry['name'] for entry in manifest] if name in brushes]),
    'currencyBrush': bool(currency_resource),
    'characterStatusGap': layout['characterStatus']['gap'],
    'runtime': runtime,
    'failures': failures,
}
(root / 'output/widget-art/editor-validation-report.json').write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(report, ensure_ascii=False))
if failures:
    raise RuntimeError('; '.join(failures))
