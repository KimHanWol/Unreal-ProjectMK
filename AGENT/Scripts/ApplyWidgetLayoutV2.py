"""Apply authored-widget spacing that cannot be expressed by the shared native builders."""
import json
from pathlib import Path

import unreal


ROOT = Path(unreal.Paths.project_dir())


def load_widget(asset_path, widget_name):
    asset_name = asset_path.rsplit('/', 1)[-1]
    object_path = f'{asset_path}.{asset_name}:WidgetTree.{widget_name}'
    widget = unreal.load_object(None, object_path)
    if not widget:
        raise RuntimeError('Missing widget: ' + object_path)
    return widget


character_asset = '/Game/UI/BP/W_CharacterInfo'
bars = load_widget(character_asset, 'VerticalBox_133')
bars_slot = bars.get_editor_property('slot')
layout = bars_slot.get_editor_property('layout_data')
offsets = layout.offsets
offsets.left = 204.0
offsets.top = 72.0
offsets.right = 280.0
offsets.bottom = 68.0
layout.offsets = offsets
bars_slot.set_editor_property('layout_data', layout)

health = load_widget(character_asset, 'W_HealthBar')
health_slot = health.get_editor_property('slot')
health_slot.set_editor_property('padding', unreal.Margin(0.0, 0.0, 0.0, 4.0))

currency_asset = '/Game/UI/BP/W_Currency'
currency_text = load_widget(currency_asset, 'Text_Coin')
currency_text.get_editor_property('slot').set_editor_property('padding', unreal.Margin(6.0, 0.0, 0.0, 0.0))

saved_assets = []
for asset_path in [character_asset, currency_asset]:
    blueprint = unreal.load_asset(asset_path)
    unreal.EditorAssetLibrary.save_loaded_asset(blueprint, False)
    saved_assets.append(asset_path)

style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
currency_texture = unreal.load_asset('/Game/Asset/ArtV1/Items/T_Coin')
if not style or not currency_texture:
    raise RuntimeError('Missing visual style or currency texture')
brushes = dict(style.get_editor_property('brushes'))
currency_brush = unreal.SlateBrush()
currency_brush.set_editor_property('resource_object', currency_texture)
currency_brush.set_editor_property('draw_as', unreal.SlateBrushDrawType.IMAGE)
brushes['Currency'] = currency_brush
style.set_editor_property('brushes', brushes)
unreal.EditorAssetLibrary.save_loaded_asset(style)

report = {
    'characterStatus': {
        'portraitRight': 194.570572,
        'barsLeft': offsets.left,
        'gap': offsets.left - 194.570572,
        'barsSize': [offsets.right, offsets.bottom],
        'healthBottomPadding': 4.0,
    },
    'currencyTextLeftPadding': 6.0,
    'savedAssets': saved_assets,
}
output = ROOT / 'output/widget-art/layout-v2-application.json'
output.parent.mkdir(parents=True, exist_ok=True)
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps(report, ensure_ascii=False))
