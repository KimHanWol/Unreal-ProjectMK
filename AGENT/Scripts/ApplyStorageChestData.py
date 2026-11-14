"""Register the reusable storage icon, placeable chest item, and buried loot settings."""
import json
import unreal

from pathlib import Path
folder = '/Game/Asset/StorageChestV3'
task = unreal.AssetImportTask()
for key, value in dict(filename=str(Path(unreal.Paths.project_dir()) / 'Content/Asset/StorageChestV3/Source/StorageChest.png'), destination_path=folder,
    destination_name='T_StorageChest', automated=True, replace_existing=True, save=True).items():
    task.set_editor_property(key, value)
unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks([task])
texture = unreal.load_asset(folder + '/T_StorageChest')
assert texture is not None
texture.set_editor_property('filter', unreal.TextureFilter.TF_NEAREST)
texture.set_editor_property('mip_gen_settings', unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS)
texture.set_editor_property('compression_settings', unreal.TextureCompressionSettings.TC_EDITOR_ICON)
texture.set_editor_property('lod_group', unreal.TextureGroup.TEXTUREGROUP_PIXELS2D)
assert unreal.EditorAssetLibrary.save_loaded_asset(texture, False)
path = folder + '/S_StorageChest'
tools = unreal.AssetToolsHelpers.get_asset_tools()
sprite = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset('S_StorageChest', folder, unreal.PaperSprite, unreal.PaperSpriteFactory())
dimensions = unreal.Vector2D(texture.blueprint_get_size_x(), texture.blueprint_get_size_y())
for key, value in dict(source_texture=texture, source_uv=unreal.Vector2D(0, 0), source_dimension=dimensions,
    source_texture_dimension=dimensions, pixels_per_unreal_unit=2.0,
    sprite_collision_domain=unreal.SpriteCollisionMode.NONE,
    default_material=unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial')).items():
    sprite.set_editor_property(key, value)
geometry = sprite.get_editor_property('render_geometry')
geometry.set_editor_property('geometry_type', unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
sprite.set_editor_property('render_geometry', geometry)
assert unreal.EditorAssetLibrary.save_loaded_asset(sprite, False)
table = unreal.load_asset('/Game/Data/DataTable/ItemDataTable')
rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
row = next((entry for entry in rows if entry['Name'] == 'StorageChest'), None)
if row is None:
    row = dict(next(entry for entry in rows if entry['Name'] == 'Workbench'))
    rows.append(row)
row.update(Name='StorageChest', DisplayName='보관 상자',
    Description='바닥에 설치하고 F를 눌러 아이템 10칸을 보관합니다. 폭파하면 상자와 내용물이 각각 바닥에 떨어집니다.',
    bIsOre=False, bIsInventoryItem=True, bIsSplitDrop=False, SellPrice=0, MaxStackCount=10,
    ItemIcon=sprite.get_path_name(), BlockOverlaySprite='None', ConsumableEffect='PlaceStorageChest',
    EffectValue=0, EffectDuration=0, PlaceBlockKey='None')
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False))
assert unreal.EditorAssetLibrary.save_loaded_asset(table, False)

settings = unreal.load_asset('/Game/Data/DataAsset/DA_GameSetting')
generation = settings.get_editor_property('mineable_level_generation')
for key, value in dict(buried_chest_chance_per_block=0.0006, buried_chest_max_count=16,
    buried_chest_minimum_depth=6, buried_chest_minimum_spacing=8).items():
    generation.set_editor_property(key, value)
generation.set_editor_property('chest_loot_roll_count', unreal.Int32Interval(min=2, max=4))
loot = []
known = {entry['Name'] for entry in rows}
for key, weight, low, high in [('Ore_Coal', 0.25, 3, 8), ('Ore_Copper', 0.24, 2, 6), ('Ore_Iron', 0.18, 2, 4),
    ('OxygenCapsule', 0.12, 1, 2), ('RepairKit', 0.10, 1, 2), ('SupportBeam', 0.08, 1, 3), ('RecallFlare', 0.03, 1, 1)]:
    assert key in known, key
    loot.append(unreal.BlockSpawnableItemData(spawnable_item_key=key, ore_spawn_weight=weight, min_spawn_item_count=low, max_spawn_item_count=high))
generation.set_editor_property('chest_loot_table', loot)
generation.set_editor_property('guaranteed_structure_patterns', [unreal.MineStructurePattern.ORE_VAULT, unreal.MineStructurePattern.HARVEST_SLAB, unreal.MineStructurePattern.BAIT_TRAP])
settings.set_editor_property('mineable_level_generation', generation)
assert unreal.EditorAssetLibrary.save_loaded_asset(settings, False)
print('Applied storage chest item, native-size sprite, seven existing loot items, and three guaranteed structure patterns. No shop recipe was added.')

blocks = unreal.load_asset('/Game/Data/DataTable/BlockDataTable')
rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(blocks))
rock = next(entry for entry in rows if entry['Name'] == 'Rock')
for name, index, art, support, falling in [('SurfaceFoundation', 901, rock['TileSprite'], True, False), ('StorageChest', 902, sprite.get_path_name(), False, True)]:
    row = next((entry for entry in rows if entry['Name'] == name), None)
    if row is None:
        row = dict(rock)
        rows.append(row)
    row.update(Name=name, TileIndex=index, TileSprite=art, bIsMineable=False, bHasCollision=True,
        bFallsWhenUnmineable=falling, bIsSupportAnchor=support, bIsExplosionProof=True,
        bIsPassableByCharacter=falling, bUseTerrainEdges=bool(rock.get('bUseTerrainEdges', False)) if support else False, bNeedTobeHide=False,
        BlockDurability=1000.0, SpawnableItemDataList=[])
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(blocks, json.dumps(rows, ensure_ascii=False))
assert unreal.EditorAssetLibrary.save_loaded_asset(blocks, False)
style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
brushes = dict(style.get_editor_property('brushes'))
storage_key = next(key for key in brushes if str(key) == 'IconStorage')
brush = brushes[storage_key]
brush.set_editor_property('resource_object', texture)
brushes[storage_key] = brush
style.set_editor_property('brushes', brushes)
assert unreal.EditorAssetLibrary.save_loaded_asset(style, False)
settings.set_editor_property('oxygen_pickup_arc_height', 14.0)
settings.set_editor_property('oxygen_pickup_display_size', 8.0)
assert unreal.EditorAssetLibrary.save_loaded_asset(settings, False)

beam = next(entry for entry in rows if entry['Name'] == 'SupportBeam')
beam['RecoveryItemKey'] = 'SupportBeam'
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(blocks, json.dumps(rows, ensure_ascii=False))
assert unreal.EditorAssetLibrary.save_loaded_asset(blocks, False)
