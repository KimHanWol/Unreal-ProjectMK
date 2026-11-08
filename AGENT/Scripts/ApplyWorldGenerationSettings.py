"""Apply editable broad-world dimensions, existing terrain layers, and purposeful encounters."""
import json
import unreal

table = unreal.load_asset('/Game/Data/DataTable/BlockDataTable')
rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
dirt = next(row for row in rows if row['Name'] == 'Dirt')
ice = next(row for row in rows if row['Name'] == 'Ice')
ice.update(TileIndex=19, bIsMineable=True, bHasCollision=True, bNeedTobeHide=False,
    BlockDurability=180.0, bUseTerrainEdges=True, TerrainBlendPriority=30,
    TileSprite='/Game/Asset/ArtV2/Terrain/S_Ice.S_Ice',
    SpawnableItemDataList=dirt['SpawnableItemDataList'])
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False))
assert unreal.EditorAssetLibrary.save_loaded_asset(table, False)

settings = unreal.load_asset('/Game/Data/DataAsset/DA_GameSetting')
generation = settings.get_editor_property('mineable_level_generation')
for name, value in {
    'width_in_blocks':128, 'depth_in_blocks':128, 'start_column':0,
    'outer_rock_thickness':4, 'shop_column':70, 'surface_block_key':'Dirt_Grass',
    'falling_structure_surface_margin':8, 'rock_block_ratio':0.03, 'empty_block_ratio':0.08,
    'bGenerateOrePuzzles':True, 'bGenerateShortcuts':True, 'bGenerateCombatTraps':True,
    'structure_monster_row_key':'DrillCrawler',
}.items():
    generation.set_editor_property(name, value)
layers = []
for key, depth in [('Dirt',0),('Sand',32),('Ice',72)]:
    layer = unreal.MineDepthLayer(block_key=key, start_depth=depth, transition_depth=12 if depth else 0)
    layers.append(layer)
generation.set_editor_property('depth_layers', layers)
generation.set_editor_property('empty_pocket_height_range', unreal.Int32Interval(min=2,max=3))
settings.set_editor_property('mineable_level_generation', generation)
assert unreal.EditorAssetLibrary.save_loaded_asset(settings, False)
print('Applied 128x128 world, 12-row dirt/sand/ice transitions, and three reward-bearing encounter purposes.')
