"""Import and connect the density-aware geology revision, preserving ArtV1 assets."""
import json
import math
from pathlib import Path
import runpy
import unreal

root=Path(unreal.Paths.project_dir())
runpy.run_path(str(root/'AGENT/Scripts/ImportArtV1.py'),init_globals={'ART_VERSION':'ArtV2'})
base='/Game/Asset/ArtV2'
report={'blocks':[],'ores':[],'background':{}}
for table_name in ['BlockDataTable','ItemDataTable']:
    table=unreal.load_asset('/Game/Data/DataTable/'+table_name)
    rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    for row in rows:
        name=row['Name']
        if table_name=='BlockDataTable' and name in ['Dirt','Dirt_Grass','Rock','Sand','Ice']:
            row['TileSprite']=unreal.load_asset(base+'/Terrain/S_'+name).get_path_name()
            report['blocks'].append(name)
        elif table_name=='ItemDataTable' and name.startswith('Ore_') and name!='Ore_Test':
            row['BlockOverlaySprite']=unreal.load_asset(base+'/Ores/S_'+name).get_path_name()
            report['ores'].append(name)
    assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows,ensure_ascii=False))
    unreal.EditorAssetLibrary.save_loaded_asset(table)
    (root/'output/art-v2'/f'{table_name}-after.json').write_text(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table),encoding='utf-8')

actors=unreal.get_editor_subsystem(unreal.EditorActorSubsystem).get_all_level_actors()
actor=next(a for a in actors if a.get_actor_label()=='ArtV1_Background_Underground')
component=actor.get_component_by_class(unreal.PaperTileMapComponent)
old_map=component.get_editor_property('tile_map')
old_size=old_map.get_editor_property('tile_width')/old_map.get_editor_property('pixels_per_unreal_unit')
old_width=old_map.get_editor_property('map_width')*old_size
old_height=old_map.get_editor_property('map_height')*old_size
old_location=actor.get_actor_location()
left=old_location.x-old_size*0.5
top=old_location.z+old_size*0.5
texture=unreal.load_asset(base+'/Backgrounds/T_Underground')
tileset_path=base+'/Backgrounds/TS_Underground'
tileset=unreal.load_asset(tileset_path) if unreal.EditorAssetLibrary.does_asset_exist(tileset_path) else unreal.AssetToolsHelpers.get_asset_tools().create_asset('TS_Underground',base+'/Backgrounds',unreal.PaperTileSet,unreal.PaperTileSetFactory())
tileset.set_editor_property('tile_sheet',texture)
tileset.set_editor_property('tile_size',unreal.IntPoint(512,512))
unreal.EditorAssetLibrary.save_loaded_asset(tileset)
width=math.ceil(old_width/256);height=math.ceil(old_height/256)
actor.set_actor_location(unreal.Vector(left+128,old_location.y,top-128),False,False)
component.create_new_tile_map(width,height,512,512,2.0,True)
component.set_collision_enabled(unreal.CollisionEnabled.NO_COLLISION)
component.set_editor_property('translucency_sort_priority',-100)
component.set_material(0,unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial'))
component.get_editor_property('tile_map').set_editor_property('sprite_collision_domain',unreal.SpriteCollisionMode.NONE)
info=unreal.PaperTileInfo()
info.set_editor_property('tile_set',tileset)
for y in range(height):
    for x in range(width):
        flags=((1<<31) if x%2 else 0)|((1<<30) if y%2 else 0)
        info.set_editor_property('packed_tile_index',flags-(1<<32) if flags>=(1<<31) else flags)
        component.set_tile(x,y,0,info)
component.rebuild_collision()
unreal.EditorAssetLibrary.save_asset('/Game/Level/Main',False)
report['background']={'texture':texture.get_path_name(),'tilePixels':512,'pixelsPerUnit':2,'worldTileSize':256,'mapSize':[width,height],'mirroredX':True,'mirroredY':True}
(root/'output/art-v2/application-report.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report))
