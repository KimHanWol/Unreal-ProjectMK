"""Validate active PIE terrain connectivity and layered material assignments."""
import json
from pathlib import Path
import unreal

world=unreal.EditorLevelLibrary.get_game_world()
assert world, 'Run in PIE after loading terrain.'
blocks=unreal.GameplayStatics.get_all_actors_of_class(world,unreal.load_class(None,'/Script/ProjectMK.BlockBase'))
terrain=[]
failures=[]
for block in blocks:
    components={c.get_name():c for c in block.get_components_by_class(unreal.PaperSpriteComponent)}
    base=components.get('PaperSprite')
    sprite=base.get_sprite() if base else None
    if sprite and '/ArtV2/Terrain/' in sprite.get_path_name() and base.is_visible():
        terrain.append((block,components))
occupied={(round(b.get_actor_location().x*2),round(b.get_actor_location().z*2)):(b,c) for b,c in terrain}
priorities={'S_Dirt':10,'S_Dirt_Grass':10,'S_Sand':20,'S_Ice':30,'S_Rock':40}
masks={}
overlays=0
transitions=0
for block,components in terrain:
    location=block.get_actor_location()
    key=(round(location.x*2),round(location.z*2))
    expected=sum(1<<i for i,(x,z) in enumerate([(-32,0),(32,0),(0,32),(0,-32)]) if (key[0]+x,key[1]+z) not in occupied)
    actual=block.get_editor_property('terrain_exposed_edges')
    priority=priorities[components['PaperSprite'].get_sprite().get_name()]
    expected_transition=0
    for index,(x,z) in enumerate([(-32,0),(32,0),(0,32),(0,-32)]):
        neighbor=occupied.get((key[0]+x,key[1]+z))
        if neighbor and priorities[neighbor[1]['PaperSprite'].get_sprite().get_name()]>priority:
            expected_transition|=1<<index
    transition=block.get_editor_property('terrain_transition_edges')
    transitions+=bool(transition)
    if transition!=expected_transition:
        failures.append({'block':block.get_name(),'expectedTransition':expected_transition,'actualTransition':transition})
    masks[str(actual)]=masks.get(str(actual),0)+1
    if actual!=expected:
        failures.append({'block':block.get_name(),'expected':expected,'actual':actual})
    shape=actual|transition
    expected_path=f'/Game/Asset/ArtV2/Materials/MI_TerrainEdges_{shape:02d}'
    overlay_material=unreal.load_asset(expected_path)
    base=components['PaperSprite']
    material=base.get_material(0)
    if transition and isinstance(material,unreal.MaterialInstanceDynamic):
        for index,side in enumerate(['Left','Right','Top','Bottom']):
            if material.get_scalar_parameter_value(side)!=float(bool(shape & (1<<index))) or material.get_scalar_parameter_value('Fill'+side)!=float(bool(transition & (1<<index))):
                failures.append({'block':block.get_name(),'parameters':side})
    elif transition or material!=overlay_material:
        failures.append({'block':block.get_name(),'material':str(material)})
    ore=components.get('ItemSprite')
    if ore and ore.get_sprite() and '/Ores/' in ore.get_sprite().get_path_name():
        overlays+=1
        if ore.get_material(0)!=overlay_material:
            failures.append({'block':block.get_name(),'oreMask':'mismatch'})
    for component in block.get_components_by_class(unreal.PaperFlipbookComponent):
        if component.get_name()=='DamageFlipbook' and component.get_material(0)!=overlay_material:
            failures.append({'block':block.get_name(),'damageMask':'mismatch'})
        if component.get_name()=='DestroyFlipbook' and '/Materials/MI_TerrainEdges_' in component.get_material(0).get_path_name():
            failures.append({'block':block.get_name(),'debrisMask':'unexpected'})
report={'terrainBlocks':len(terrain),'oreOverlays':overlays,'materialTransitions':transitions,'masks':masks,'failures':failures}
output=Path(unreal.Paths.project_dir(),'output/terrain-edges')
output.mkdir(parents=True,exist_ok=True)
(output/'runtime-validation.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
print(json.dumps(report))
assert terrain and not failures
