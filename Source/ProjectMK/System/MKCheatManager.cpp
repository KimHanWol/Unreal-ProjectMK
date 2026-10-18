// LINK

#include "ProjectMK/System/MKCheatManager.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Modules/ModuleManager.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/ConsumableComponent.h"
#include "ProjectMK/Component/CraftingComponent.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Controller/MKPlayerController.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Helper/Utils/GameplayAbilityUtils.h"
#include "ProjectMK/System/SkillDebugUtils.h"

namespace
{
	void SendCheatMessage(const UObject* WorldContextObject, const FString& Message)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
		{
			PlayerController->ClientMessage(Message);
		}

		UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
	}

	TArray<FName> CollectRowNamesFromAssetRegistry(const FString& ExpectedRowStructName, const UScriptStruct* ExpectedRowStruct)
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		FARFilter Filter;
		Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> DataTableAssets;
		AssetRegistryModule.Get().GetAssets(Filter, DataTableAssets);

		TSet<FName> UniqueRowNames;
		for (const FAssetData& DataTableAsset : DataTableAssets)
		{
			const FString RowStructPath = DataTableAsset.TagsAndValues.FindTag(TEXT("RowStructure")).AsString();
			if (RowStructPath.Contains(ExpectedRowStructName) == false)
			{
				continue;
			}

			const UDataTable* DataTable = Cast<UDataTable>(DataTableAsset.GetAsset());
			if (::IsValid(DataTable) == false || DataTable->GetRowStruct() != ExpectedRowStruct)
			{
				continue;
			}

			for (const FName& RowName : DataTable->GetRowNames())
			{
				UniqueRowNames.Add(RowName);
			}
		}

		TArray<FName> SortedRowNames = UniqueRowNames.Array();
		SortedRowNames.Sort(FNameLexicalLess());
		return SortedRowNames;
	}

	TArray<FName> CollectItemRowNamesFromAssetRegistry()
	{
		return CollectRowNamesFromAssetRegistry(TEXT("ItemDataTableRow"), FItemDataTableRow::StaticStruct());
	}
}

AMKCharacter* UMKCheatManager::GetLocalPlayerCharacter() const
{
	return Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
}

AMKPlayerController* UMKCheatManager::GetMKPlayerController() const
{
	return Cast<AMKPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
}

void UMKCheatManager::DamagePlayer(float Damage)
{
	if (Damage <= 0.f)
	{
		return;
	}

	AMKCharacter* PlayerCharacter = GetLocalPlayerCharacter();
	if (::IsValid(PlayerCharacter) == false)
	{
		return;
	}

	FGameplayAbilityUtils::ApplyDamage(PlayerCharacter->GetAbilitySystemComponent(), nullptr, Damage);
}

void UMKCheatManager::DrainOxygen(float OxygenAmount)
{
	if (OxygenAmount <= 0.f)
	{
		return;
	}

	AMKCharacter* PlayerCharacter = GetLocalPlayerCharacter();
	if (::IsValid(PlayerCharacter) == false)
	{
		return;
	}

	FGameplayAbilityUtils::ApplyOxygen(PlayerCharacter->GetAbilitySystemComponent(), -OxygenAmount);
}

void UMKCheatManager::GiveItem(FName ItemUID, int32 ItemCount)
{
	if (ItemUID.IsNone())
	{
		SendCheatMessage(this, TEXT("GiveItem usage: GiveItem <ItemUID> [ItemCount]"));
		SendCheatMessage(this, TEXT("Available item keys:"));
		for (const FName& ItemRowName : CollectItemRowNamesFromAssetRegistry())
		{
			SendCheatMessage(this, ItemRowName.ToString());
		}
		return;
	}

	if (ItemCount <= 0)
	{
		SendCheatMessage(this, TEXT("GiveItem failed: ItemCount must be greater than 0."));
		return;
	}

	AMKCharacter* PlayerCharacter = GetLocalPlayerCharacter();
	if (::IsValid(PlayerCharacter) == false)
	{
		SendCheatMessage(this, TEXT("GiveItem failed: PlayerCharacter is invalid."));
		return;
	}

	UInventoryComponent* InventoryComponent = PlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent) == false)
	{
		SendCheatMessage(this, TEXT("GiveItem failed: InventoryComponent is invalid."));
		return;
	}

	if (CollectItemRowNamesFromAssetRegistry().Contains(ItemUID) == false)
	{
		SendCheatMessage(this, FString::Printf(TEXT("GiveItem failed: '%s' is not a valid item key."), *ItemUID.ToString()));
		return;
	}

	if (InventoryComponent->AddItem(ItemUID, ItemCount) == false)
	{
		SendCheatMessage(this, FString::Printf(TEXT("GiveItem failed: could not add '%s' x%d."), *ItemUID.ToString(), ItemCount));
		return;
	}

	SendCheatMessage(this, FString::Printf(TEXT("Added '%s' x%d"), *ItemUID.ToString(), ItemCount));
}

void UMKCheatManager::GetItem(FName ItemUID, int32 ItemCount)
{
	GiveItem(ItemUID, ItemCount);
}

void UMKCheatManager::ResetMineableLevel(int32 GenerationSeed)
{
	UWorld* World = GetWorld();
	if (::IsValid(World) == false)
	{
		SendCheatMessage(this, TEXT("ResetMineableLevel failed: World is invalid."));
		return;
	}

	ULevelManagerSubsystem* LevelManagerSubsystem = World->GetSubsystem<ULevelManagerSubsystem>();
	if (::IsValid(LevelManagerSubsystem) == false)
	{
		SendCheatMessage(this, TEXT("ResetMineableLevel failed: LevelManagerSubsystem is invalid."));
		return;
	}

	AMKCharacter* PlayerCharacter = GetLocalPlayerCharacter();
	if (::IsValid(PlayerCharacter) == false)
	{
		SendCheatMessage(this, TEXT("ResetMineableLevel failed: PlayerCharacter is invalid."));
		return;
	}

	LevelManagerSubsystem->ResetMineableLevel(GenerationSeed);
	PlayerCharacter->RespawnAtSpawnLocation();
	SendCheatMessage(
		this,
		FString::Printf(TEXT("Mineable level reset with seed %d."), LevelManagerSubsystem->GetLastGenerationSeed()));
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

void UMKCheatManager::SetSkillDebugLog(bool bEnabled)
{
	FSkillDebugUtils::SetSkillDebugEnabled(bEnabled);
	SendCheatMessage(this, FString::Printf(TEXT("Skill debug log %s"), bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void UMKCheatManager::ToggleSkillDebugLog()
{
	const bool bEnabled = FSkillDebugUtils::ToggleSkillDebugEnabled();
	SendCheatMessage(this, FString::Printf(TEXT("Skill debug log %s"), bEnabled ? TEXT("enabled") : TEXT("disabled")));
}

void UMKCheatManager::ToggleInventoryWidget()
{
	AMKPlayerController* PlayerController = GetMKPlayerController();
	if (::IsValid(PlayerController) == false)
	{
		return;
	}

	PlayerController->ToggleInventoryWidget();
}

void UMKCheatManager::OpenShop()
{
	AMKPlayerController* PlayerController = GetMKPlayerController();
	if (::IsValid(PlayerController) == false || PlayerController->TryOpenShop() == false)
	{
		SendCheatMessage(this, TEXT("OpenShop failed: Menu is invalid."));
	}
}

void UMKCheatManager::UseQuickSlot(int32 SlotNumber)
{
	AMKCharacter* LocalCharacter = GetLocalPlayerCharacter();
	UConsumableComponent* ConsumableComponent = ::IsValid(LocalCharacter) ? LocalCharacter->GetConsumableComponent() : nullptr;
	UInventoryComponent* InventoryComponent = ::IsValid(LocalCharacter) ? LocalCharacter->FindComponentByClass<UInventoryComponent>() : nullptr;
	const TArray<FInventorySlotData> SlotDataList = ::IsValid(InventoryComponent) ? InventoryComponent->GetInventorySlotDataList() : TArray<FInventorySlotData>();
	const FName SlotItemUID = SlotDataList.IsValidIndex(SlotNumber - 1) ? SlotDataList[SlotNumber - 1].ItemUID : NAME_None;
	const bool bUsed = ::IsValid(ConsumableComponent) && ConsumableComponent->TryUseInventorySlot(SlotNumber - 1);
	const int32 RemainingCount = ::IsValid(InventoryComponent) ? InventoryComponent->GetItemCount(SlotItemUID) : 0;
	SendCheatMessage(this, FString::Printf(TEXT("UseQuickSlot %d (%s): %s, remaining %d"), SlotNumber, *SlotItemUID.ToString(), bUsed ? TEXT("used") : TEXT("failed"), RemainingCount));
}

void UMKCheatManager::UnlockRecipe(FName RecipeKey)
{
	AMKCharacter* LocalCharacter = GetLocalPlayerCharacter();
	UCraftingComponent* CraftingComponent = ::IsValid(LocalCharacter) ? LocalCharacter->GetCraftingComponent() : nullptr;
	const bool bUnlocked = ::IsValid(CraftingComponent) && CraftingComponent->TryUnlockRecipe(RecipeKey);
	SendCheatMessage(this, FString::Printf(TEXT("UnlockRecipe %s: %s"), *RecipeKey.ToString(), bUnlocked ? TEXT("unlocked") : TEXT("failed")));
}

void UMKCheatManager::CraftRecipe(FName RecipeKey)
{
	AMKCharacter* LocalCharacter = GetLocalPlayerCharacter();
	UCraftingComponent* CraftingComponent = ::IsValid(LocalCharacter) ? LocalCharacter->GetCraftingComponent() : nullptr;
	const bool bCrafted = ::IsValid(CraftingComponent) && CraftingComponent->TryCraftRecipe(RecipeKey);
	SendCheatMessage(this, FString::Printf(TEXT("CraftRecipe %s: %s"), *RecipeKey.ToString(), bCrafted ? TEXT("crafted") : TEXT("failed")));
}

