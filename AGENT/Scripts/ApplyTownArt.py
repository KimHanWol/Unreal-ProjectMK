"""Import town/transport art and register editable factory offers and prototype quest settings."""
import json
from pathlib import Path
import unreal

root = Path(unreal.Paths.project_dir())
assets = unreal.AssetToolsHelpers.get_asset_tools()

def import_sprite(name, source, folder):
    task = unreal.AssetImportTask()
    for key,value in dict(filename=str(root/source),destination_path=folder,destination_name='T_'+name,automated=True,replace_existing=True,save=True).items():
        task.set_editor_property(key,value)
    assets.import_asset_tasks([task])
    texture = unreal.load_asset(folder+'/T_'+name)
    for key,value in dict(filter=unreal.TextureFilter.TF_NEAREST,mip_gen_settings=unreal.TextureMipGenSettings.TMGS_NO_MIPMAPS,compression_settings=unreal.TextureCompressionSettings.TC_EDITOR_ICON,lod_group=unreal.TextureGroup.TEXTUREGROUP_PIXELS2D).items():
        texture.set_editor_property(key,value)
    unreal.EditorAssetLibrary.save_loaded_asset(texture,False)
    path=folder+'/S_'+name
    sprite=unreal.load_asset(path) if unreal.EditorAssetLibrary.does_asset_exist(path) else assets.create_asset('S_'+name,folder,unreal.PaperSprite,unreal.PaperSpriteFactory())
    size=unreal.Vector2D(texture.blueprint_get_size_x(),texture.blueprint_get_size_y())
    for key,value in dict(source_texture=texture,source_uv=unreal.Vector2D(0,0),source_dimension=size,source_texture_dimension=size,pixels_per_unreal_unit=2.0,sprite_collision_domain=unreal.SpriteCollisionMode.NONE,default_material=unreal.load_asset('/Paper2D/MaskedUnlitSpriteMaterial')).items():
        sprite.set_editor_property(key,value)
    geometry=sprite.get_editor_property('render_geometry')
    geometry.set_editor_property('geometry_type',unreal.SpritePolygonMode.SOURCE_BOUNDING_BOX)
    sprite.set_editor_property('render_geometry',geometry)
    unreal.EditorAssetLibrary.save_loaded_asset(sprite,False)
    return sprite

manifest=json.loads((root/'Content/Asset/TownArtV1/Source/manifest.json').read_text(encoding='utf-8-sig'))
sprites={entry['name']:import_sprite(entry['name'],entry['file'],'/Game/Asset/TownArtV1') for entry in manifest}
support=import_sprite('SupportBeam','Content/Asset/SupportClimbV1/Source/SupportBeam.png','/Game/Asset/SupportClimbV1')
style=unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
style.set_editor_property('town_sprites',sprites)
unreal.EditorAssetLibrary.save_loaded_asset(style,False)

items=unreal.load_asset('/Game/Data/DataTable/ItemDataTable')
rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(items))
template=dict(next(row for row in rows if row['Name']=='StorageChest'))
def item(key,label,description,effect,art,limit):
    row=next((row for row in rows if row['Name']==key),None)
    if row is None:row=dict(template);rows.append(row)
    row.update(Name=key,DisplayName=label,Description=description,bIsOre=False,bIsInventoryItem=True,bIsSplitDrop=False,SellPrice=0,MaxStackCount=limit,ItemIcon=art.get_path_name(),BlockOverlaySprite='None',ConsumableEffect=effect,EffectValue=0,EffectDuration=0,PlaceBlockKey='None')
item('Rail','철도','평지·대각선 바닥에 설치합니다. 분기와 순환 연결은 할 수 없습니다.','PlaceRail',sprites['Rail'],99)
item('MineCart','광산 열차','철도 근처에서 설치합니다. 몸으로 천천히 밀고 경사에서는 중력으로 이동합니다. 앞에서 상자를 사용하면 장착합니다.','PlaceMineCart',sprites['MineCart'],10)
item('AutoMineCart','전자동 광산 열차','철도에 설치하고 밀면 그 방향으로 자동 운행합니다. 오르막도 이동하며 선로 끝에서 멈춥니다.','PlaceAutoMineCart',sprites['AutoMineCart'],10)
item('Relic_SteadyBelt','광부의 중심추','획득하면 항상 적용됩니다. 몬스터 접촉 시 피해는 받지만 넉백으로 튕겨 나가지 않습니다.','None',sprites['SteadyBelt'],1)
next(row for row in rows if row['Name']=='Relic_SteadyBelt')['bIsInventoryItem']=False
beam=next(row for row in rows if row['Name']=='SupportBeam')
beam.update(ItemIcon=support.get_path_name(),Description='쌓아 올릴 수 있는 광산 지지대입니다. 근처에서 W를 누르면 기둥을 따라 올라가 윗면에 설 수 있습니다.')
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(items,json.dumps(rows,ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(items,False)
blocks=unreal.load_asset('/Game/Data/DataTable/BlockDataTable')
rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(blocks))
beam=next(row for row in rows if row['Name']=='SupportBeam')
beam.update(TileSprite=support.get_path_name(),bIsMineable=False,bIsSupportAnchor=True,bIsPassableByCharacter=True,bUseTerrainEdges=False,RecoveryItemKey='SupportBeam')
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(blocks,json.dumps(rows,ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(blocks,False)

recipes=unreal.load_asset('/Game/Data/DataTable/ShopRecipeDataTable')
rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(recipes))
template=dict(next(row for row in rows if row['OfferType']=='Recipe'))
for key,label,price,unlock,ingredients,count in [
 ('Rail','철도',5,20,[('Ore_Iron',2),('Ore_Copper',1)],4),
 ('MineCart','광산 열차',60,60,[('Ore_Iron',6),('Ore_Copper',3)],1),
 ('AutoMineCart','전자동 광산 열차',180,120,[('Ore_Iron',8),('Ore_Copper',5),('Ore_Gold',2)],1)]:
    for direct in [True,False]:
        name='RailFactory_'+key if direct else key+'Recipe'
        row=next((row for row in rows if row['Name']==name),None)
        if row is None:row=dict(template);rows.append(row)
        row.update(Name=name,OfferType='Item' if direct else 'Recipe',CatalogKey='RailFactory',DisplayName=label,Description='철도 공장에서 구입하거나 설계도를 배워 작업대에서 제작합니다.',ShortDescription='철도 운송',BlueprintItemKey='Blueprint',PurchasePrice=price,PurchaseLimit=20 if key=='Rail' else 2,RequiredItems=[dict(ItemKey=k,ItemCount=n) for k,n in ingredients],ResultItemKey=key,ResultItemCount=1 if direct else count,UnlockPrice=unlock)
assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(recipes,json.dumps(rows,ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(recipes,False)
settings=unreal.load_asset('/Game/Data/DataAsset/DA_GameSetting')
town=settings.get_editor_property('town_facilities')
quests=[]
for title,key,count,reward,depth in [('갱구 보수용 석탄을 부탁해요.','Ore_Coal',8,30,0),('공구 수리에 쓸 구리가 필요해요.','Ore_Copper',6,45,8),('지하 설비를 보강할 철을 구해 주세요.','Ore_Iron',5,60,16)]:
    quest=unreal.TownQuestDefinition()
    for field,value in dict(title=title,required_item=key,required_count=count,reward_coins=reward,minimum_depth=depth).items():quest.set_editor_property(field,value)
    quests.append(quest)
town.set_editor_property('quests',quests)
settings.set_editor_property('town_facilities',town)
unreal.EditorAssetLibrary.save_loaded_asset(settings,False)
print('Applied town facilities, rail/manual/automatic cart items and recipes, open-bottom X support, and three editable resident quests.')
