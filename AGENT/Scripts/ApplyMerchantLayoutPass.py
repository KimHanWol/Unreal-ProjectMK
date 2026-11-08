"""Apply blueprint catalog presentation and move only the HUD's HP/O2 group upward."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
tools = unreal.AssetToolsHelpers.get_asset_tools()
folder = '/Game/Asset/WidgetArtV2/UI'
task = unreal.AssetImportTask()
for name, value in {
    'filename': str(root / 'Content/Asset/Texture/Items/item669.png'),
    'destination_path': folder, 'destination_name': 'T_Blueprint',
    'automated': True, 'replace_existing': True, 'save': True,
}.items():
    task.set_editor_property(name, value)
tools.import_asset_tasks([task])
texture = unreal.load_asset(folder + '/T_Blueprint')
texture.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_UI)
assert unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
sprite_path = folder + '/S_Blueprint'
sprite = unreal.load_asset(sprite_path) if unreal.EditorAssetLibrary.does_asset_exist(sprite_path) else tools.create_asset('S_Blueprint', folder, unreal.PaperSprite, unreal.PaperSpriteFactory())
sprite.set_editor_property('source_texture', texture)
sprite.set_editor_property('source_uv', unreal.Vector2D(0, 0))
dimensions = unreal.Vector2D(texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
sprite.set_editor_property('source_dimension', dimensions)
sprite.set_editor_property('source_texture_dimension', dimensions)
sprite.set_editor_property('pixels_per_unreal_unit', 2.0)
sprite.set_editor_property('sprite_collision_domain', unreal.SpriteCollisionMode.NONE)
geometry = sprite.get_editor_property('render_geometry')
geometry.set_editor_property('geometry_type', unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
sprite.set_editor_property('render_geometry', geometry)
assert unreal.EditorAssetLibrary.save_loaded_asset(sprite, False)

table = unreal.load_asset('/Game/Data/DataTable/ItemDataTable')
items = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
blueprint = next((row for row in items if row['Name'] == 'Blueprint'), None)
if blueprint is None:
    blueprint = dict(next(row for row in items if row['Name'] == 'Insurance'))
    blueprint.update(Name='Blueprint', DisplayName='설계도', Description='새로운 제작 레시피를 해금하는 설계도입니다.',
        bIsInventoryItem=True, MaxStackCount=1, ItemIcon=sprite.get_path_name(),
        ConsumableEffect='None', EffectValue=0, EffectDuration=0, SellPrice=0)
    items.append(blueprint)
else:
    blueprint['ItemIcon'] = sprite.get_path_name()
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(items, ensure_ascii=False))
assert unreal.EditorAssetLibrary.save_loaded_asset(table, False)

recipes = unreal.load_asset('/Game/Data/DataTable/ShopRecipeDataTable')
rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(recipes))
summaries = {
    'OxygenCapsule': '최대 산소의 40% 회복', 'RepairKit': '체력 50 회복',
    'RecallFlare': '준비 후 지상으로 귀환', 'SupportBeam': '빈 공간에 버팀목 설치',
    'CushionGel': '블록 낙하 충격 방지', 'BlastCharge': '주변 블록을 폭파',
    'Insurance': '사망 시 광물 손실 1회 방지', 'Ore_Test': '제작 테스트용 광물',
}
for row in rows:
    row['BlueprintItemKey'] = 'Blueprint'
    if not row.get('ShortDescription'):
        row['ShortDescription'] = summaries.get(row['ResultItemKey'], '')
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(recipes, json.dumps(rows, ensure_ascii=False))
assert unreal.EditorAssetLibrary.save_loaded_asset(recipes, False)

asset = '/Game/UI/BP/W_CharacterInfo'
bars = unreal.load_object(None, asset + '.W_CharacterInfo:WidgetTree.VerticalBox_133')
slot = bars.get_editor_property('slot')
layout = slot.get_editor_property('layout_data')
layout.offsets = unreal.Margin(168, 80, 260, 64)
slot.set_editor_property('layout_data', layout)
widget = unreal.load_asset(asset)
unreal.BlueprintEditorLibrary.compile_blueprint(widget)
assert unreal.EditorAssetLibrary.save_loaded_asset(widget, False)
print('Applied existing scroll icon, blueprint item, eight recipe summaries, and HP/O2 offset 92 -> 80.')
