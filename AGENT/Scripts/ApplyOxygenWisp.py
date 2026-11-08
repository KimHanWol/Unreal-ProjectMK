"""Reuse the approved pickup silhouette with cyan vertex color, independent of item-icon color."""
import unreal

folder = '/Game/Asset/ArtV2/Materials'
path = folder + '/M_OxygenWisp'
assets = unreal.AssetToolsHelpers.get_asset_tools()
material = unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else assets.create_asset('M_OxygenWisp', folder, unreal.Material, unreal.MaterialFactoryNew())
editing = unreal.MaterialEditingLibrary
editing.delete_all_material_expressions(material)
material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
material.set_editor_property('two_sided', True)
sample = editing.create_material_expression(material, unreal.MaterialExpressionSpriteTextureSampler, -500, 150)
sample.set_editor_property('texture', unreal.load_asset('/Game/Asset/ArtV1/Effects/T_Pickup1'))
tint = editing.create_material_expression(material, unreal.MaterialExpressionVertexColor, -500, -100)
alpha = editing.create_material_expression(material, unreal.MaterialExpressionMultiply, -250, 150)
assert editing.connect_material_expressions(sample, 'A', alpha, 'A')
assert editing.connect_material_expressions(tint, 'A', alpha, 'B')
assert editing.connect_material_property(tint, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
assert editing.connect_material_property(alpha, '', unreal.MaterialProperty.MP_OPACITY)
editing.recompile_material(material)
assert unreal.EditorAssetLibrary.save_loaded_asset(material, False)
style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
style.set_editor_property('oxygen_pickup_material', material)

core_path = folder + '/M_OxygenWispCore'
core_material = unreal.load_asset(core_path) if unreal.EditorAssetLibrary.does_asset_exist(core_path) else assets.create_asset('M_OxygenWispCore', folder, unreal.Material, unreal.MaterialFactoryNew())
editing.delete_all_material_expressions(core_material)
core_material.set_editor_property('blend_mode', unreal.BlendMode.BLEND_TRANSLUCENT)
core_material.set_editor_property('shading_model', unreal.MaterialShadingModel.MSM_UNLIT)
core_material.set_editor_property('two_sided', True)
uv = editing.create_material_expression(core_material, unreal.MaterialExpressionTextureCoordinate, -600, 0)
color = editing.create_material_expression(core_material, unreal.MaterialExpressionVertexColor, -600, 200)
core = editing.create_material_expression(core_material, unreal.MaterialExpressionCustom, -300, 0)
core.set_editor_property('description', 'Pixel stepped sky-blue oxygen core')
core.set_editor_property('output_type', unreal.CustomMaterialOutputType.CMOT_FLOAT4)
core.set_editor_property('code', '''
float2 P = floor(UV * 32.0) - 15.5;
float R = length(P);
float A = R < 10.0 ? 0.95 : (R < 12.0 ? 0.30 : (R < 14.0 ? 0.08 : 0.0));
float3 C = R < 3.0 ? float3(0.88, 1.0, 1.0) : (R < 7.0 ? float3(0.38, 0.86, 1.0) : float3(0.10, 0.58, 0.88));
return float4(C * Tint, A * TintAlpha);
''')
inputs = []
for name in ['UV', 'Tint', 'TintAlpha']:
    value = unreal.CustomInput()
    value.set_editor_property('input_name', name)
    inputs.append(value)
core.set_editor_property('inputs', inputs)
assert editing.connect_material_expressions(uv, '', core, 'UV')
assert editing.connect_material_expressions(color, '', core, 'Tint')
assert editing.connect_material_expressions(color, 'A', core, 'TintAlpha')
rgb = editing.create_material_expression(core_material, unreal.MaterialExpressionComponentMask, 0, 0)
for channel in ['r','g','b']: rgb.set_editor_property(channel, True)
rgb.set_editor_property('a', False)
opacity = editing.create_material_expression(core_material, unreal.MaterialExpressionComponentMask, 0, 150)
for channel in ['r','g','b']: opacity.set_editor_property(channel, False)
opacity.set_editor_property('a', True)
assert editing.connect_material_expressions(core, '', rgb, '')
assert editing.connect_material_expressions(core, '', opacity, '')
assert editing.connect_material_property(rgb, '', unreal.MaterialProperty.MP_EMISSIVE_COLOR)
assert editing.connect_material_property(opacity, '', unreal.MaterialProperty.MP_OPACITY)
editing.recompile_material(core_material)
assert unreal.EditorAssetLibrary.save_loaded_asset(core_material, False)
sprite_folder = '/Game/Asset/ArtV2/Effects'
sprite_path = sprite_folder + '/S_OxygenWispCore'
sprite = unreal.load_asset(sprite_path) if unreal.EditorAssetLibrary.does_asset_exist(sprite_path) else assets.create_asset('S_OxygenWispCore', sprite_folder, unreal.PaperSprite, unreal.PaperSpriteFactory())
sprite.set_editor_property('source_texture', unreal.load_asset('/Game/Asset/ArtV1/Effects/T_Pickup1'))
sprite.set_editor_property('source_uv', unreal.Vector2D(0,0))
sprite.set_editor_property('source_dimension', unreal.Vector2D(20,20))
sprite.set_editor_property('source_texture_dimension', unreal.Vector2D(20,20))
sprite.set_editor_property('pixels_per_unreal_unit', 2.0)
sprite.set_editor_property('sprite_collision_domain', unreal.SpriteCollisionMode.NONE)
geometry = sprite.get_editor_property('render_geometry')
geometry.set_editor_property('geometry_type', unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
sprite.set_editor_property('render_geometry', geometry)
sprite.set_editor_property('default_material', core_material)
assert unreal.EditorAssetLibrary.save_loaded_asset(sprite, False)
style.set_editor_property('oxygen_pickup_core', sprite)
assert unreal.EditorAssetLibrary.save_loaded_asset(style, False)
print('Applied pixel cyan core, diminishing trails, and the existing pickup sparkle silhouette.')
