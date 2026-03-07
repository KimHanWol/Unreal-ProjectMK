// LINK

#include "ProjectMK/System/MKCheatManager.h"

#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Controller/MKPlayerController.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Helper/Utils/DamageableUtil.h"

void UMKCheatManager::DamagePlayer(float Damage)
{
	if (Damage <= 0.f)
	{
		return;
	}

	AMKCharacter* PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(PlayerCharacter) == false)
	{
		return;
	}

	FDamageableUtil::ApplyDamage(PlayerCharacter->GetAbilitySystemComponent(), nullptr, Damage);
}

void UMKCheatManager::GiveItem(FName ItemUID, int32 ItemCount)
{
	if (ItemUID.IsNone() || ItemCount <= 0)
	{
		return;
	}

	AMKCharacter* PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(PlayerCharacter) == false)
	{
		return;
	}

	UInventoryComponent* InventoryComponent = PlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent) == false)
	{
		return;
	}

	const UDataManager* DataManager = UDataManager::Get(this);
	if (::IsValid(DataManager) == false)
	{
		return;
	}

	const bool bIsValidItem =
		DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemUID) != nullptr ||
		DataManager->GetDataTableRow<FEquipmentItemDataTableRow>(EDataTableType::EquipmentItem, ItemUID) != nullptr;
	if (bIsValidItem == false)
	{
		return;
	}

	InventoryComponent->AddItem(ItemUID, ItemCount);
}

void UMKCheatManager::EnableBlockDebugNumbers()
{
	UWorld* World = GetWorld();
	if (::IsValid(World) == false)
	{
		return;
	}

	ULevelManagerSubsystem* LevelManagerSubsystem = World->GetSubsystem<ULevelManagerSubsystem>();
	if (::IsValid(LevelManagerSubsystem) == false)
	{
		return;
	}

	LevelManagerSubsystem->SetBlockDebugNumbersEnabled(true);
}

void UMKCheatManager::ToggleShopTestWidget()
{
	AMKPlayerController* PlayerController = Cast<AMKPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (::IsValid(PlayerController) == false)
	{
		return;
	}

	PlayerController->ToggleShopTestWidget();
}
