"""Preserve authored corners, crop a scrollbar thumb sprite, and classify character passives."""
import json
import unreal

style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
brushes = {str(key): value for key, value in style.get_editor_property('brushes').items()}
for role, width, height, corner in [('Panel', 160, 120, 32), ('InsetPanel', 128, 80, 16), ('Minimap', 96, 96, 32)]:
    brush = brushes[role]
    brush.set_editor_property('margin', unreal.Margin(corner/width, corner/height, corner/width, corner/height))
    brushes[role] = brush
folder = '/Game/Asset/WidgetArtV1/UI'
path = folder + '/S_ScrollThumbSlice'
tools = unreal.AssetToolsHelpers.get_asset_tools()
sprite = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset('S_ScrollThumbSlice', folder, unreal.PaperSprite, unreal.PaperSpriteFactory())
texture = unreal.load_asset('/Game/Asset/ArtV1/Symbols/T_ScrollHandle')
sprite.set_editor_property('source_texture', texture)
sprite.set_editor_property('source_uv', unreal.Vector2D(11, 11))
sprite.set_editor_property('source_dimension', unreal.Vector2D(10, 10))
sprite.set_editor_property('source_texture_dimension', unreal.Vector2D(32, 32))
sprite.set_editor_property('sprite_collision_domain', unreal.SpriteCollisionMode.NONE)
geometry = sprite.get_editor_property('render_geometry')
geometry.set_editor_property('geometry_type', unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
sprite.set_editor_property('render_geometry', geometry)
unreal.EditorAssetLibrary.save_loaded_asset(sprite)
thumb = unreal.WidgetLibrary.make_brush_from_texture(texture, 12, 32)
thumb.set_editor_property('resource_object', sprite)
thumb.set_editor_property('draw_as', unreal.SlateBrushDrawType.BOX)
thumb.set_editor_property('margin', unreal.Margin(0.2, 0.2, 0.2, 0.2))
brushes['ScrollHandle'] = thumb
style.set_editor_property('brushes', brushes)
unreal.EditorAssetLibrary.save_loaded_asset(style)
table = unreal.load_asset('/Game/Data/DataTable/SkillDataTable')
rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
passives = {'CharacterSkill_MinerDiscipline', 'CharacterSkill_RecklessDig', 'CharacterSkill_SpeedRunner', 'CharacterSkill_HeavyCarrier'}
for row in rows:
    if row['Name'] in passives:
        row['AcquisitionType'] = 'CharacterPassive'
unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows))
unreal.EditorAssetLibrary.save_loaded_asset(table)
print('Applied intact panel corners, independent scrollbar thumb, and four character-passive rows.')
