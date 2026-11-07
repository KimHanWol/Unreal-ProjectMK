"""Create shared edge materials without altering source textures or sprite geometry."""
import json
from pathlib import Path
import unreal

root=Path(unreal.Paths.project_dir())
folder='/Game/Asset/ArtV2/Materials'
path=folder+'/M_TerrainEdges'
assets=unreal.AssetToolsHelpers.get_asset_tools()
editing=unreal.MaterialEditingLibrary
material=unreal.find_object(None,path+'.M_TerrainEdges') or unreal.load_asset(path)
if not material:
    material=assets.create_asset('M_TerrainEdges',folder,unreal.Material,unreal.MaterialFactoryNew())
editing.delete_all_material_expressions(material)
material.set_editor_property('blend_mode',unreal.BlendMode.BLEND_MASKED)
material.set_editor_property('shading_model',unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided',True)
material.set_editor_property('opacity_mask_clip_value',0.5)

def node(cls,x,y):
    return editing.create_material_expression(material,cls,x,y)

uv=node(unreal.MaterialExpressionTextureCoordinate,-900,0)
sample=node(unreal.MaterialExpressionSpriteTextureSampler,-900,180)
sample.set_editor_property('texture',unreal.load_asset('/Game/Asset/ArtV2/Terrain/T_Dirt'))
tint=node(unreal.MaterialExpressionVertexColor,-900,380)
custom=node(unreal.MaterialExpressionCustom,-300,0)
custom.set_editor_property('description','Rough material boundaries; central 24x24 unchanged')
custom.set_editor_property('code',(root/'AGENT/Art/TerrainEdges.hlsl').read_text(encoding='utf-8-sig'))
custom.set_editor_property('output_type',unreal.CustomMaterialOutputType.CMOT_FLOAT4)
inputs=[]
for name in ['UV','Source','Tint','TintAlpha','EdgeSeed','Left','Right','Top','Bottom','FillLeft','FillRight','FillTop','FillBottom','LeftTexture','RightTexture','TopTexture','BottomTexture']:
    value=unreal.CustomInput()
    value.set_editor_property('input_name',name)
    inputs.append(value)
custom.set_editor_property('inputs',inputs)
assert editing.connect_material_expressions(uv,'',custom,'UV')
assert editing.connect_material_expressions(sample,'RGBA',custom,'Source')
assert editing.connect_material_expressions(tint,'',custom,'Tint')
assert editing.connect_material_expressions(tint,'A',custom,'TintAlpha')
seed=node(unreal.MaterialExpressionScalarParameter,-900,550)
seed.set_editor_property('parameter_name','EdgeSeed')
seed.set_editor_property('use_custom_primitive_data',True)
seed.set_editor_property('primitive_data_index',0)
assert editing.connect_material_expressions(seed,'',custom,'EdgeSeed')
for index,name in enumerate(['Left','Right','Top','Bottom','FillLeft','FillRight','FillTop','FillBottom']):
    parameter=node(unreal.MaterialExpressionScalarParameter,-650,500+index*150)
    parameter.set_editor_property('parameter_name',name)
    parameter.set_editor_property('default_value',0.0)
    assert editing.connect_material_expressions(parameter,'',custom,name)
for index,side in enumerate(['Left','Right','Top','Bottom']):
    texture=node(unreal.MaterialExpressionTextureObjectParameter,-1100,750+index*200)
    texture.set_editor_property('parameter_name',side+'Texture')
    texture.set_editor_property('texture',unreal.load_asset('/Game/Asset/ArtV2/Terrain/T_Dirt'))
    assert editing.connect_material_expressions(texture,'',custom,side+'Texture')
rgb=node(unreal.MaterialExpressionComponentMask,0,0)
rgb.set_editor_property('r',True)
rgb.set_editor_property('g',True)
rgb.set_editor_property('b',True)
rgb.set_editor_property('a',False)
alpha=node(unreal.MaterialExpressionComponentMask,0,200)
for channel in ['r','g','b']:
    alpha.set_editor_property(channel,False)
alpha.set_editor_property('a',True)
assert editing.connect_material_expressions(custom,'',rgb,'')
assert editing.connect_material_expressions(custom,'',alpha,'')
assert editing.connect_material_property(rgb,'',unreal.MaterialProperty.MP_EMISSIVE_COLOR)
assert editing.connect_material_property(alpha,'',unreal.MaterialProperty.MP_OPACITY_MASK)
editing.recompile_material(material)
unreal.EditorAssetLibrary.save_loaded_asset(material)
instances=[]
for mask in range(16):
    name=f'MI_TerrainEdges_{mask:02d}'
    instance_path=folder+'/'+name
    instance=unreal.find_object(None,instance_path+'.'+name) or unreal.load_asset(instance_path)
    if not instance:
        instance=assets.create_asset(name,folder,unreal.MaterialInstanceConstant,unreal.MaterialInstanceConstantFactoryNew())
    editing.set_material_instance_parent(instance,material)
    for index,side in enumerate(['Left','Right','Top','Bottom']):
        editing.set_material_instance_scalar_parameter_value(instance,side,float(bool(mask & (1<<index))))
    editing.update_material_instance(instance)
    unreal.EditorAssetLibrary.save_loaded_asset(instance)
    instances.append(instance)

if not globals().get('MATERIALS_ONLY',False):
    style=unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
    style.set_editor_property('terrain_edge_materials',instances)
    unreal.EditorAssetLibrary.save_loaded_asset(style)
    table=unreal.load_asset('/Game/Data/DataTable/BlockDataTable')
    rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    enabled=['Dirt','Dirt_Grass','Rock','Sand','Ice']
    priorities={'Dirt':10,'Dirt_Grass':10,'Sand':20,'Ice':30,'Rock':40}
    for row in rows:
        row['bUseTerrainEdges']=row['Name'] in enabled
        row['TerrainBlendPriority']=priorities.get(row['Name'],0)
    assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows))
    unreal.EditorAssetLibrary.save_loaded_asset(table)
    (root/'output/art-v2/BlockDataTable-after.json').write_text(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table),encoding='utf-8')
    report=root/'output/terrain-edges'
    report.mkdir(parents=True,exist_ok=True)
    (report/'application.json').write_text(json.dumps({'materials':[i.get_path_name() for i in instances],'blocks':enabled},indent=2)+'\n',encoding='utf-8')
print('Terrain edge materials ready:',len(instances))
