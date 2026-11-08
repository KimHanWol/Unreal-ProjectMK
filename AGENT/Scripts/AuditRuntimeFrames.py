"""Capture widget geometry and data for ProjectMK UI regression diagnosis."""
import json
from pathlib import Path
import unreal

report = {'characterAsset': [], 'runtimeBars': [], 'runtimeSlots': [], 'style': {}}
for obj in unreal.ObjectIterator(unreal.Widget):
    path = obj.get_path_name()
    if '/Game/UI/BP/W_CharacterInfo.W_CharacterInfo:WidgetTree.' in path:
        row = {'name': obj.get_name(), 'class': obj.get_class().get_name()}
        slot = obj.get_editor_property('slot')
        if slot:
            row['slotClass'] = slot.get_class().get_name()
            try:
                row['layout'] = str(slot.get_editor_property('layout_data'))
            except Exception:
                pass
        report['characterAsset'].append(row)
    if '/Engine/Transient.' in path and isinstance(obj, unreal.ProgressBar):
        report['runtimeBars'].append({'path': path, 'percent': obj.get_editor_property('percent'), 'size': str(unreal.SlateLibrary.get_local_size(obj.get_cached_geometry()))})
    if '/Engine/Transient.' in path and obj.get_name() in ['Slot', 'SelectedSlot', 'Keycap', 'IconImage', 'Title', 'Desc']:
        row = {'path': path, 'size': str(unreal.SlateLibrary.get_local_size(obj.get_cached_geometry())), 'visibility': str(obj.get_visibility())}
        slot = obj.get_editor_property('slot')
        if isinstance(slot, unreal.OverlaySlot):
            row['horizontal'] = str(slot.get_editor_property('horizontal_alignment'))
            row['vertical'] = str(slot.get_editor_property('vertical_alignment'))
        if isinstance(obj, unreal.TextBlock):
            row['text'] = str(obj.get_text())
        report['runtimeSlots'].append(row)
style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
for name, brush in style.get_editor_property('brushes').items():
    report['style'][str(name)] = {'size': str(brush.get_editor_property('image_size')), 'margin': str(brush.get_editor_property('margin'))}
output = Path(unreal.Paths.project_saved_dir()) / 'AgentLogs/runtime-ui-audit.json'
output.write_text(json.dumps(report, ensure_ascii=False, indent=2), encoding='utf-8')
print(json.dumps({'characterAsset': report['characterAsset'], 'runtimeBars': report['runtimeBars'], 'output': str(output)}, ensure_ascii=False))
