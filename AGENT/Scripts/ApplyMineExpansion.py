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

manifest=json.loads((root/'Content/Asset/MineExpansionV1/Source/manifest.json').read_text(encoding='utf-8-sig'))
sprites={entry['name']:import_sprite(entry['name'],entry['file'],'/Game/Asset/MineExpansionV1') for entry in manifest}
style=unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
town_sprites=dict(style.get_editor_property('town_sprites'))
town_sprites['IndustrialBackdrop']=sprites['IndustrialBackdrop']
style.set_editor_property('town_sprites',town_sprites)
unreal.EditorAssetLibrary.save_loaded_asset(style,False)

def update_table(path, transform):
    table=unreal.load_asset(path)
    rows=json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
    transform(rows)
    assert unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table,json.dumps(rows,ensure_ascii=False))
    unreal.EditorAssetLibrary.save_loaded_asset(table,False)

def blocks(rows):
    next(r for r in rows if r['Name']=='Shop')['TileSprite']=sprites['Shop'].get_path_name()
    template=dict(next(r for r in rows if r['Name']=='Dirt'))
    next_tile=max(r['TileIndex'] for r in rows)+1
    for key,durability,drops in [('TreeTrunk',12,[dict(SpawnableItemKey='Wood',OreSpawnWeight=1.0,MinSpawnItemCount=2,MaxSpawnItemCount=2)]),('TreeLeaves',1,[])]:
        row=next((r for r in rows if r['Name']==key),None)
        if row is None:
            row=dict(template,Name=key,TileIndex=next_tile);rows.append(row);next_tile+=1
        row.update(TileSprite=sprites[key].get_path_name(),bHasCollision=True,bIsMineable=True,bIsSupportAnchor=False,bIsExplosionProof=False,bIsPassableByCharacter=False,bUseTerrainEdges=False,bFallsWhenUnmineable=False,bDamagesTerrainOnImpact=False,bDropOnCollapse=(key=='TreeTrunk'),RecoveryItemKey='None',BlockDurability=durability,SpawnableItemDataList=drops,bNeedTobeHide=False)

def items(rows):
    row=next((r for r in rows if r['Name']=='Wood'),None)
    if row is None:row=dict(next(r for r in rows if r['Name']=='Ore_Coal'),Name='Wood');rows.append(row)
    row.update(DisplayName='목재',Description='벌목한 나무에서 얻은 목재입니다. 광산 지지대와 철도·열차 제작에 사용합니다.',bIsOre=False,bIsInventoryItem=True,bIsSplitDrop=False,SellPrice=2,MaxStackCount=99,ItemIcon=sprites['Wood'].get_path_name(),BlockOverlaySprite='None',ConsumableEffect='None',EffectValue=0,EffectDuration=0,PlaceBlockKey='None')
    next(r for r in rows if r['Name']=='Rail')['Description']='평지·대각선 바닥을 따라 설치하는 철도입니다. 분기와 순환 연결은 할 수 없습니다. 지지를 잃으면 떨어집니다.'
    next(r for r in rows if r['Name']=='SupportBeam')['Description']='쌓아 올릴 수 있는 광산 지지대입니다. 근처에서 W를 누르면 기둥을 따라 올라가 윗면에 설 수 있습니다.'

def recipes(rows):
    for row in rows:
        if 'F 홀드 회수' in row.get('ShortDescription',''):row['ShortDescription']=row['ShortDescription'].replace(' · F 홀드 회수','')
        if row.get('OfferType')!='Recipe':continue
        amount={'SupportBeam':2,'Rail':2,'MineCart':4,'AutoMineCart':4}.get(row.get('ResultItemKey'))
        if amount:
            required=[r for r in row['RequiredItems'] if r.get('ItemKey')!='Wood']
            required.append(dict(ItemKey='Wood',ItemCount=amount));row['RequiredItems']=required
update_table('/Game/Data/DataTable/ItemDataTable',items)
update_table('/Game/Data/DataTable/BlockDataTable',blocks)
update_table('/Game/Data/DataTable/ShopRecipeDataTable',recipes)
settings=unreal.load_asset('/Game/Data/DataAsset/DA_GameSetting')
town=settings.get_editor_property('town_facilities')
quest=unreal.TownQuestDefinition()
for field,value in dict(title='갱도 드릴 크롤러 퇴치',target_monster='DrillCrawler',required_count=3,reward_coins=75,minimum_depth=0).items():quest.set_editor_property(field,value)
town.set_editor_property('hunt_quests',[quest])
settings.set_editor_property('town_facilities',town)
forestry=settings.get_editor_property('forestry')
for field,value in dict(enabled=True,columns=[14,22,30],regrow_seconds=90).items():forestry.set_editor_property(field,value)
settings.set_editor_property('forestry',forestry)
unreal.EditorAssetLibrary.save_loaded_asset(settings,False)
print('Applied supply shop, industrial background, tree blocks and Wood recipes, and editable DrillCrawler board contract.')
