"""Connect ArtV1 to existing game data after compiling the visual integration."""
import json
import math
from pathlib import Path
import unreal

ROOT = Path(unreal.Paths.project_dir())
BASE = '/Game/Asset/ArtV1'
tools = unreal.AssetToolsHelpers.get_asset_tools()
report = {'tables':{},'effects':[], 'backgrounds':[]}

def asset(group, prefix, name):
    result = unreal.load_asset(BASE + '/' + group + '/' + prefix + name)
    if not result:
        raise RuntimeError('Missing asset: '+group+'/'+name)
    return result

for table_name in ['BlockDataTable','ItemDataTable','SkillDataTable']:
    table = unreal.load_asset('/Game/Data/DataTable/' + table_name)
    rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    count = 0
    for row in rows:
        name = row['Name']
        if table_name == 'BlockDataTable' and name != 'Border':
            row['TileSprite'] = asset('Symbols' if name == 'Shop' else 'Terrain','S_',name).get_path_name()
            count += 1
        elif table_name == 'ItemDataTable':
            row['ItemIcon'] = asset('Items','S_',name).get_path_name()
            if name.startswith('Ore_') and name != 'Ore_Test':
                row['BlockOverlaySprite'] = asset('Ores','S_',name).get_path_name()
            count += 1
        elif table_name == 'SkillDataTable' and name != 'NewRow':
            key = name.removeprefix('CharacterSkill_').removeprefix('Skill_')
            if key[-2:-1] == '_':
                key = key[:-2]
            row['IconPath'] = asset('Skills','T_',key).get_path_name()
            count += 1
    if not unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows,ensure_ascii=False)):
        raise RuntimeError('Data table import failed: '+table_name)
    unreal.EditorAssetLibrary.save_loaded_asset(table)
    report['tables'][table_name] = count
    (ROOT/'output/art-v1'/f'{table_name}-after.json').write_text(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table),encoding='utf-8')

def flipbook(name, group, frame_names, fps):
    path = BASE + '/Effects/FB_' + name
    result = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else tools.create_asset('FB_'+name,BASE+'/Effects',unreal.PaperFlipbook,unreal.PaperFlipbookFactory())
    frames=[]
    for frame_name in frame_names:
        frame=unreal.PaperFlipbookKeyFrame()
        frame.set_editor_property('sprite',asset(group,'S_',frame_name))
        frame.set_editor_property('frame_run',1)
        frames.append(frame)
    result.set_editor_property('key_frames',frames)
    result.set_editor_property('frames_per_second',fps)
    result.set_editor_property('default_material',unreal.load_asset('/Paper2D/TranslucentUnlitSpriteMaterial'))
    result.set_editor_property('collision_source',unreal.FlipbookCollisionMode.NO_COLLISION)
    unreal.EditorAssetLibrary.save_loaded_asset(result)
    report['effects'].append(result.get_path_name())
    return result

damage = flipbook('BlockDamage','Damage',['Damage20','Damage40','Damage60','Damage80'],5)
effects={name:flipbook(name,'Effects',[name+str(i) for i in range(4)],10 if name=='Explosion' else 12) for name in ['BlockBreak','DrillContact','Explosion','Pickup']}
for component_name, value in [('DamageFlipbook',damage),('DestroyFlipbook',effects['BlockBreak'])]:
    component=unreal.load_object(None,'/Game/Actor/Block/BP_Block.Default__BP_Block_C:'+component_name)
    if not component:
        raise RuntimeError('Missing block template '+component_name)
    component.set_editor_property('source_flipbook',value)
    component.set_editor_property('relative_scale3d',unreal.Vector(1,1,1))
    component.set_editor_property('sprite_color',unreal.LinearColor(1,1,1,1))
block_blueprint=unreal.load_asset('/Game/Actor/Block/BP_Block')
unreal.BlueprintEditorLibrary.compile_blueprint(block_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(block_blueprint,False)

style_path=BASE+'/DA_VisualStyle'
style=unreal.load_asset(style_path) if unreal.EditorAssetLibrary.does_asset_exist(style_path) else tools.create_asset('DA_VisualStyle',BASE,unreal.VisualStyleDataAsset,unreal.DataAssetFactory())
brushes={}
for name in ['Panel','Slot','SelectedSlot','Tooltip','Button','ButtonHover','ButtonPressed','ButtonDisabled','Portrait','Minimap','BarFrame','OxygenFrame','HealthFill','OxygenFill','Keycap','SkillNode']:
    brush=unreal.SlateBrush()
    brush.set_editor_property('resource_object',asset('UI','T_',name))
    brush.set_editor_property('draw_as',unreal.SlateBrushDrawType.BOX)
    brush.set_editor_property('margin',unreal.Margin(0.1875,0.1875,0.1875,0.1875))
    brushes[name]=brush
for name in ['PlayerMarker','ShopMarker','Lock','Check','Close','Warning','Heart','Oxygen','ArrowDown','ArrowRight','Sparkle','Settings','ScrollHandle','Crafting']:
    brush=unreal.SlateBrush()
    brush.set_editor_property('resource_object',asset('Symbols','T_',name))
    brush.set_editor_property('draw_as',unreal.SlateBrushDrawType.IMAGE)
    brushes[name]=brush
style.set_editor_property('brushes',brushes)
style.set_editor_property('cushion_overlay',asset('Terrain','S_','Cushion'))
style.set_editor_property('effects',effects)
unreal.EditorAssetLibrary.save_loaded_asset(style)
manager_blueprint=unreal.load_asset('/Game/Core/BP_DataManager')
unreal.BlueprintEditorLibrary.compile_blueprint(manager_blueprint)
manager=unreal.get_default_object(unreal.load_class(None,'/Game/Core/BP_DataManager.BP_DataManager_C'))
manager.set_editor_property('visual_style_data_asset',style)
unreal.BlueprintEditorLibrary.compile_blueprint(manager_blueprint)
unreal.EditorAssetLibrary.save_loaded_asset(manager_blueprint,False)

# Decorative maps have no collision, and are excluded from gameplay map discovery.
subsystem=unreal.get_editor_subsystem(unreal.EditorActorSubsystem)
actors=subsystem.get_all_level_actors()
game_maps=[a for a in actors if isinstance(a,unreal.PaperTileMapActor) and 'ArtBackground' not in [str(t) for t in a.tags]]
if len(game_maps)!=1:
    raise RuntimeError('Expected one gameplay tile map; refusing ambiguous backdrop placement.')
origin,extent=game_maps[0].get_actor_bounds(False)
left_edge=origin.x-extent.x-256
game_component=game_maps[0].get_component_by_class(unreal.PaperTileMapComponent)
game_map=game_component.get_editor_property('tile_map')
block_rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(unreal.load_asset('/Game/Data/DataTable/BlockDataTable')))
grass_index=next(row['TileIndex'] for row in block_rows if row['Name']=='Dirt_Grass')
surface_cells=[(x,y) for y in range(game_map.get_editor_property('map_height')) for x in range(game_map.get_editor_property('map_width')) if game_component.get_tile(x,y,0).get_editor_property('tile_set') and game_component.get_tile(x,y,0).get_editor_property('packed_tile_index') & 0xffff == grass_index]
if not surface_cells:
    raise RuntimeError('No grass surface found for backdrop alignment.')
x,y=min(surface_cells,key=lambda cell:cell[1])
center=game_component.get_tile_center_position(x,y,0,True)
ground=center.z+game_map.get_editor_property('tile_height')/game_map.get_editor_property('pixels_per_unreal_unit')*0.5
for name,tile_pixels in [('Surface',512),('Underground',128)]:
    pixels_per_unit=2.0
    tile_world_size=tile_pixels/pixels_per_unit
    width=math.ceil((extent.x*2+512)/tile_world_size)
    height=1 if name=='Surface' else math.ceil(extent.z*2/tile_world_size)+1
    left=left_edge+tile_world_size*0.5
    z=ground+tile_world_size*0.5 if name=='Surface' else ground-tile_world_size*0.5
    tileset_path=BASE+'/Backgrounds/TS_'+name
    tileset=unreal.load_asset(tileset_path) if unreal.EditorAssetLibrary.does_asset_exist(tileset_path) else tools.create_asset('TS_'+name,BASE+'/Backgrounds',unreal.PaperTileSet,unreal.PaperTileSetFactory())
    tileset.set_editor_property('tile_sheet',asset('Backgrounds','T_',name))
    tileset.set_editor_property('tile_size',unreal.IntPoint(tile_pixels,tile_pixels))
    unreal.EditorAssetLibrary.save_loaded_asset(tileset)
    label='ArtV1_Background_'+name
    actor=next((a for a in actors if a.get_actor_label()==label),None)
    if not actor:
        actor=subsystem.spawn_actor_from_class(unreal.PaperTileMapActor,unreal.Vector(left,-100,z))
    actor.set_actor_location(unreal.Vector(left,-100,z),False,False)
    actor.set_actor_label(label)
    actor.set_editor_property('tags',['ArtBackground'])
    actor.set_folder_path('ArtV1/Backgrounds')
    component=actor.get_component_by_class(unreal.PaperTileMapComponent)
    component.create_new_tile_map(width,height,tile_pixels,tile_pixels,pixels_per_unit,True)
    component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
    component.set_editor_property('translucency_sort_priority',-100)
    component.set_material(0,unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial'))
    tilemap=component.get_editor_property('tile_map')
    tilemap.set_editor_property('sprite_collision_domain',unreal.SpriteCollisionMode.NONE)
    info=unreal.PaperTileInfo()
    info.set_editor_property('tile_set',tileset)
    for y in range(height):
        for x in range(width):
            # PaperTileLayer.h: horizontal bit 31, vertical bit 30; exposed as signed int32.
            flags=((1<<31) if x%2 else 0) | ((1<<30) if name=='Underground' and y%2 else 0)
            info.set_editor_property('packed_tile_index',flags-(1<<32) if flags >= (1<<31) else flags)
            component.set_tile(x,y,0,info)
    component.rebuild_collision()
    report['backgrounds'].append(actor.get_path_name())
unreal.EditorLoadingAndSavingUtils.save_dirty_packages(False,True)
unreal.EditorAssetLibrary.save_asset('/Game/Level/Main',False)
(ROOT/'output/art-v1/application-report.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
print(json.dumps(report,ensure_ascii=False))
