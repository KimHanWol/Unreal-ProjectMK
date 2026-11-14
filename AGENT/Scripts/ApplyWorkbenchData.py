"""Add a placeable workbench using the project's existing pixel art and data tables."""
import json
import unreal

tools = unreal.AssetToolsHelpers.get_asset_tools()
folder = '/Game/Asset/WidgetArtV2/UI'
texture = unreal.load_asset(folder + '/T_IconWorkbench')
assert texture is not None
path = folder + '/S_Workbench'
sprite = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset('S_Workbench', folder, unreal.PaperSprite, unreal.PaperSpriteFactory())
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
rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
row = next((entry for entry in rows if entry['Name'] == 'Workbench'), None)
if row is None:
    row = dict(next(entry for entry in rows if entry['Name'] == 'SupportBeam'))
    rows.append(row)
row.update(Name='Workbench', DisplayName='작업대', Description='바닥 위에 작업대를 설치합니다. 가까이에서 F를 눌러 제작할 수 있습니다.',
    bIsOre=False, bIsInventoryItem=True, bIsSplitDrop=False, SellPrice=20, MaxStackCount=10,
    ItemIcon=sprite.get_path_name(), BlockOverlaySprite='None', ConsumableEffect='PlaceWorkbench',
    EffectValue=0, EffectDuration=0, PlaceBlockKey='None')
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False))
assert unreal.EditorAssetLibrary.save_loaded_asset(table, False)

table = unreal.load_asset('/Game/Data/DataTable/ShopRecipeDataTable')
rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
row = next((entry for entry in rows if entry['Name'] == 'Workbench'), None)
if row is None:
    row = dict(next(entry for entry in rows if entry['ResultItemKey'] == 'SupportBeam'))
    rows.append(row)
row.update(Name='Workbench', DisplayName='작업대', Description='설치한 작업대 근처에서 제작 메뉴를 열 수 있습니다.',
    ShortDescription='바닥에 설치 · F 제작', BlueprintItemKey='Blueprint',
    RequiredItems=[dict(ItemKey='Ore_Copper', ItemCount=3), dict(ItemKey='Ore_Iron', ItemCount=2)],
    ResultItemKey='Workbench', ResultItemCount=1, UnlockPrice=0)
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False))
assert unreal.EditorAssetLibrary.save_loaded_asset(table, False)
print('Applied Workbench item, unlocked recipe, and reusable pixel sprite')
