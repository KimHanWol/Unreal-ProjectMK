"""Render all 16 real GPU material combinations for central-pixel regression checks."""
from pathlib import Path
import unreal

world=unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem).get_editor_world()
output=Path(unreal.Paths.project_dir(),'output/terrain-edges/gpu')
output.mkdir(parents=True,exist_ok=True)
for mask in range(16):
    material=unreal.load_asset(f'/Game/Asset/ArtV2/Materials/MI_TerrainEdges_{mask:02d}')
    target=unreal.RenderingLibrary.create_render_target2d(world,32,32,unreal.TextureRenderTargetFormat.RTF_RGBA8,unreal.LinearColor(0,0,0,0))
    unreal.RenderingLibrary.draw_material_to_render_target(world,target,material)
    unreal.RenderingLibrary.export_render_target(world,target,str(output),f'edges-{mask:02d}.png')
print('Rendered 16 terrain masks to',output)
terrain_render_jobs=[]
for mask in [1,2,4,8,15]:
    parent=unreal.load_asset(f'/Game/Asset/ArtV2/Materials/MI_TerrainEdges_{mask:02d}')
    material=unreal.MaterialLibrary.create_dynamic_material_instance(world,parent)
    for index,side in enumerate(['Left','Right','Top','Bottom']):
        material.set_scalar_parameter_value('Fill'+side,float(bool(mask & (1<<index))))
        material.set_texture_parameter_value(side+'Texture',unreal.load_asset('/Game/Asset/ArtV2/Terrain/T_Rock'))
    target=unreal.RenderingLibrary.create_render_target2d(world,32,32,unreal.TextureRenderTargetFormat.RTF_RGBA8,unreal.LinearColor(0,0,0,0))
    terrain_render_jobs.append((mask,material,target))
# Dynamic uniforms reach the render thread after creation. Warm up and capture on separate frames.
terrain_render_frame=0
def terrain_render_tick(delta):
    global terrain_render_frame
    terrain_render_frame+=1
    try:
        if terrain_render_frame<3:
            for mask,material,target in terrain_render_jobs:
                unreal.RenderingLibrary.draw_material_to_render_target(world,target,material)
        else:
            for mask,material,target in terrain_render_jobs:
                unreal.RenderingLibrary.export_render_target(world,target,str(output),f'filled-{mask:02d}.png')
            unreal.unregister_slate_post_tick_callback(terrain_render_handle)
            print('Rendered 5 material transitions')
    except Exception:
        unreal.unregister_slate_post_tick_callback(terrain_render_handle)
        raise
terrain_render_handle=unreal.register_slate_post_tick_callback(terrain_render_tick)
print('Material transition renders queued')
