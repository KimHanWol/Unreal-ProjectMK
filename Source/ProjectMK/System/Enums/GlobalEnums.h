// LINK

#pragma once

#include "CoreMinimal.h"
#include "GlobalEnums.generated.h"

UENUM(BlueprintType)
enum class EDataTableType : uint8
{
	None = 0,
	Block = 1,
	Item = 2,
	// Do not reuse this value. It preserves historic enum ordering used by older serialized assets.
	LegacyReserved = 3 UMETA(Hidden),
	Character = 4,
	ShopRecipe = 5,
	Sound = 6,
	Skill = 7,
	Monster = 8,
};

UENUM(BlueprintType)
enum class ESFXType : uint8
{
	None,
	Floating,
	Drill,
	Break_Dirt,
	Drill_Blocked,
	Block_Shake,
	// 같은 프레임에 처리된 블록 수에 따라 Single / Medium / Large 로 나눈다 (GameSettingDataAsset 의 Sound 카테고리 참고).
	Block_Mine_Single,
	Block_Mine_Medium,
	Block_Mine_Large,
	Block_Break_Single,
	Block_Break_Medium,
	Block_Break_Large,
	Block_Fall_Single,
	Block_Fall_Medium,
	Block_Fall_Large,
	Character_Hit,
	Character_Death,
	Character_Respawn,
	Oxygen_Low,
	Oxygen_Refill,
	Item_Drop,
	Item_Pickup,
	Inventory_Full,
	Magnet_Activate,
	Coin_Gain,
	Skill_Purchase,
	Skill_PurchaseFailed,
	Skill_LuckyStrike,
	Skill_ChainBreak,
	Skill_AirPocket,
	Skill_Recycler,
	Skill_FieldRepair,
	Shop_Open,
	Shop_Sell,
	Shop_Craft,
	UI_Hover,
	UI_Click,
	UI_MenuOpen,
	UI_MenuClose,
	Item_Use,
	Item_UseFailed,
	Blast_Explode,
	Recipe_Unlock,
	Recall_Start,
	Recall_Complete,
};

UENUM(BlueprintType)
enum class ESFXInstanceParamType : uint8
{
	None,
	DestroyedBlockCount,
	Max,
};

UENUM(BlueprintType)
enum class ESFXContinuousParamType : uint8
{
	None,
	HP,
	Flying,
	Drill,
};

UENUM(BlueprintType)
enum class EMenuContentsType : uint8
{
	None,
	SkillTree,
	Shop,
	Crafting,
};

/** 소모품 사용 효과. 수치는 아이템 데이터의 EffectValue / EffectDuration 으로 조정한다 */
UENUM(BlueprintType)
enum class EConsumableEffectType : uint8
{
	None,
	/** EffectValue: 최대 산소 대비 회복 비율 */
	RestoreOxygen,
	/** EffectValue: 체력 회복량 */
	RestoreHealth,
	/** EffectDuration: 움직이지 않고 기다려야 하는 시간 */
	Recall,
	/** PlaceBlockKey: 바라보는 빈칸에 설치할 블록 */
	PlaceBlock,
	/** 바라보는 블록에 발라, 그 위로 떨어진 블록의 충격을 없앤다 */
	ApplyCushion,
	/** EffectDuration: 폭발까지의 시간, EffectValue: 폭발 반경(칸) */
	PlaceBlastCharge,
	/** 소지하고 있으면 다음 사망 1회의 페널티를 막고 소모된다 */
	Insurance,
};
