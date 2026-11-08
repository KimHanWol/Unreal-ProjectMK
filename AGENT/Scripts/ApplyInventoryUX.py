"""Apply HUD-safe frame slices and editable Korean item/recipe copy without changing gameplay fields."""
import json
import unreal

style = unreal.load_asset('/Game/Asset/ArtV1/DA_VisualStyle')
brushes = {str(key): value for key, value in style.get_editor_property('brushes').items()}
for role in ('BarFrame', 'OxygenFrame'):
    brush = brushes[role]
    brush.set_editor_property('margin', unreal.Margin(34/160, 8/32, 7/160, 9/32))
    brushes[role] = brush
tooltip = brushes['Tooltip']
tooltip.set_editor_property('margin', unreal.Margin(32/128, 28/96, 32/128, 28/96))
brushes['Tooltip'] = tooltip
brushes.pop('PlayerPortrait', None)
style.set_editor_property('brushes', brushes)
unreal.EditorAssetLibrary.save_loaded_asset(style)

table = unreal.load_asset('/Game/Data/DataTable/ItemDataTable')
rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(table))
recipes = unreal.load_asset('/Game/Data/DataTable/ShopRecipeDataTable')
recipe_rows = json.loads(unreal.DataTableFunctionLibrary.export_data_table_to_json_string(recipes))
names = {
    'Coal': '석탄', 'Copper': '구리', 'Iron': '철', 'Silver': '은', 'Gold': '금',
    'Diamond': '다이아몬드', 'Ruby': '루비', 'Emerald': '에메랄드', 'Sapphire': '사파이어',
    'Amethyst': '자수정', 'Quartz': '석영', 'Uranium': '우라늄', 'Stone': '돌', 'Dirt': '흙',
    'OxygenCapsule': '산소 캡슐', 'FirstAidKit': '응급 키트', 'RecallFlare': '귀환 신호탄',
    'BlastCharge': '발파 폭약', 'SupportBeam': '버팀목', 'CushionGel': '완충 젤',
    'Insurance': '보험 증서', 'Coin': '코인', 'Oxygen': '산소',
    'Titanium': '티타늄', 'Lithium': '리튬', 'Slime': '점액', 'Chain': '사슬',
}
for recipe in recipe_rows:
    if recipe.get('DisplayName'):
        names[recipe.get('ResultItemKey', '')] = recipe['DisplayName']

for row in rows:
    key = row['Name']
    display = row.get('DisplayName', '')
    if not display or display == key or display.endswith('"' + key + '")'):
        row['DisplayName'] = names.get(key, names.get(key.removeprefix('Ore_'), key))
    if not row.get('Description'):
        if row.get('bIsOre'):
            row['Description'] = '채굴로 얻은 광물. 상점에 판매하거나 제작 재료로 사용합니다.'
        else:
            effect = str(row.get('ConsumableEffect', 'None')).split('::')[-1]
            value = row.get('EffectValue', 0)
            descriptions = {
                'RestoreHealth': f'체력을 {value:g} 회복합니다.',
                'RestoreOxygen': f'최대 산소의 {value*100:g}%를 회복합니다.',
                'Recall': '잠시 준비한 뒤 지상으로 돌아갑니다. 준비 중 이동하면 취소됩니다.',
                'PlaceBlock': '가까운 빈 공간에 버팀목을 설치합니다.',
                'ApplyCushion': '블록에 완충재를 붙여 낙하 충격을 막습니다.',
                'PlaceBlastCharge': '주변 블록을 부수는 폭약을 설치합니다.',
                'Insurance': '소지 중 사망하면 광물 손실을 한 번 막아 줍니다.',
            }
            row['Description'] = descriptions.get(effect, '제작과 보강에 사용하는 재료입니다.' if row.get('bIsInventoryItem') else '')
    if key == 'CushionGel' and '낙하 충격을 줄입니다.' in row.get('Description', ''):
        row['Description'] = '블록에 완충재를 붙여 낙하 충격을 막습니다.'

unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(table, json.dumps(rows, ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(table)
by_key = {row['Name']: row for row in rows}
for recipe in recipe_rows:
    if not recipe.get('Description'):
        recipe['Description'] = by_key.get(recipe.get('ResultItemKey'), {}).get('Description', '')
unreal.DataTableFunctionLibrary.fill_data_table_from_json_string(recipes, json.dumps(recipe_rows, ensure_ascii=False))
unreal.EditorAssetLibrary.save_loaded_asset(recipes)
print('Applied asymmetric gauge/tooltip slices and item/recipe copy:', len(rows), len(recipe_rows))
from pathlib import Path
exec((Path(unreal.Paths.project_dir()) / 'AGENT/Scripts/ApplyPopupPolish.py').read_text(encoding='utf-8-sig'), {})
