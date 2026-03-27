// LINK

#include "ProjectMK/System/MKCheatManager.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Modules/ModuleManager.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Controller/MKPlayerController.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/Core/Subsystem/LevelManagerSubsystem.h"
#include "ProjectMK/Helper/Utils/EquipmentItemDataTableUtil.h"
#include "ProjectMK/Helper/Utils/DamageableUtil.h"

namespace
{
	const TArray<EEuipmentType>& GetCheatSupportedEquipmentTypes()
	{
		static const TArray<EEuipmentType> EquipmentTypes =
		{
			EEuipmentType::Halmet,
			EEuipmentType::Armor,
			EEuipmentType::Drill,
			EEuipmentType::Balloon,
			EEuipmentType::Gloves,
			EEuipmentType::Shoes
		};

		return EquipmentTypes;
	}

	void SendCheatMessage(const UObject* WorldContextObject, const FString& Message)
	{
		if (APlayerController* PlayerController = UGameplayStatics::GetPlayerController(WorldContextObject, 0))
		{
			PlayerController->ClientMessage(Message);
		}

		UE_LOG(LogTemp, Log, TEXT("%s"), *Message);
	}

	FString GetEquipmentTypeDisplayName(EEuipmentType EquipmentType)
	{
		const UEnum* EquipmentEnum = StaticEnum<EEuipmentType>();
		if (EquipmentEnum == nullptr)
		{
			return TEXT("Unknown");
		}

		return EquipmentEnum->GetDisplayNameTextByValue(static_cast<int64>(EquipmentType)).ToString();
	}

	UInventoryComponent* GetPlayerInventoryComponent(const UObject* WorldContextObject)
	{
		AMKCharacter* PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(WorldContextObject, 0));
		if (::IsValid(PlayerCharacter) == false)
		{
			return nullptr;
		}

		return PlayerCharacter->GetComponentByClass<UInventoryComponent>();
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
	if (ItemUID.IsNone())
	{
		SendCheatMessage(this, TEXT("GetItem usage: GetItem <ItemUID> [ItemCount]"));
		SendCheatMessage(this, TEXT("Available item keys:"));
		for (const FName& ItemRowName : CollectItemRowNamesFromAssetRegistry())
		{
			SendCheatMessage(this, ItemRowName.ToString());
		}

		SendCheatMessage(this, TEXT("Available equipment item keys:"));
		for (const FName& EquipmentRowName : FEquipmentItemDataTableUtil::GetEquipmentItemRowNames(this))
		{
			SendCheatMessage(this, EquipmentRowName.ToString());
		}
		return;
	}

	if (ItemCount <= 0)
	{
		SendCheatMessage(this, TEXT("GetItem failed: ItemCount must be greater than 0."));
		return;
	}

	AMKCharacter* PlayerCharacter = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	if (::IsValid(PlayerCharacter) == false)
	{
		SendCheatMessage(this, TEXT("GetItem failed: PlayerCharacter is invalid."));
		return;
	}

	UInventoryComponent* InventoryComponent = PlayerCharacter->GetComponentByClass<UInventoryComponent>();
	if (::IsValid(InventoryComponent) == false)
	{
		SendCheatMessage(this, TEXT("GetItem failed: InventoryComponent is invalid."));
		return;
	}

	const UDataManager* DataManager = UDataManager::Get(this);
	const bool bIsValidItem =
		(::IsValid(DataManager) &&
			(
				DataManager->GetDataTableRow<FItemDataTableRow>(EDataTableType::Item, ItemUID) != nullptr ||
				DataManager->GetDataTableRow<FEquipmentItemDataTableRow>(EDataTableType::EquipmentItem, ItemUID) != nullptr
			)) ||
		CollectItemRowNamesFromAssetRegistry().Contains(ItemUID) ||
		FEquipmentItemDataTableUtil::GetEquipmentItemRowNames(this).Contains(ItemUID);
	if (bIsValidItem == false)
	{
		SendCheatMessage(this, FString::Printf(TEXT("GetItem failed: '%s' is not a valid item key."), *ItemUID.ToString()));
		return;
	}

	if (InventoryComponent->AddItem(ItemUID, ItemCount) == false)
	{
		SendCheatMessage(this, FString::Printf(TEXT("GetItem failed: could not add '%s' x%d."), *ItemUID.ToString(), ItemCount));
		return;
	}

	SendCheatMessage(this, FString::Printf(TEXT("Added '%s' x%d"), *ItemUID.ToString(), ItemCount));
}

void UMKCheatManager::GetItem(FName ItemUID, int32 ItemCount)
{
	GiveItem(ItemUID, ItemCount);
}

void UMKCheatManager::EquipItem(FName ItemUID)
{
	if (ItemUID.IsNone())
	{
		const TArray<FName> EquipmentRowNames = FEquipmentItemDataTableUtil::GetEquipmentItemRowNames(this);
		if (EquipmentRowNames.IsEmpty())
		{
			SendCheatMessage(this, TEXT("EquipItem: no equipment item keys found."));
			return;
		}

		SendCheatMessage(this, TEXT("EquipItem usage: EquipItem <ItemUID>"));
		SendCheatMessage(this, TEXT("Available equipment item keys:"));
		for (const FName& EquipmentRowName : EquipmentRowNames)
		{
			SendCheatMessage(this, EquipmentRowName.ToString());
		}
		return;
	}

	UInventoryComponent* InventoryComponent = GetPlayerInventoryComponent(this);
	if (::IsValid(InventoryComponent) == false)
	{
		SendCheatMessage(this, TEXT("EquipItem failed: InventoryComponent is invalid."));
		return;
	}

	const FEquipmentItemDataTableRow* EquipmentData = FEquipmentItemDataTableUtil::FindEquipmentItemData(this, ItemUID);
	if (EquipmentData == nullptr)
	{
		SendCheatMessage(this, FString::Printf(TEXT("EquipItem failed: '%s' is not a valid equipment item."), *ItemUID.ToString()));
		return;
	}

	if (InventoryComponent->GetItemCount(ItemUID) <= 0 && InventoryComponent->AddItem(ItemUID, 1) == false)
	{
		SendCheatMessage(this, FString::Printf(TEXT("EquipItem failed: could not add '%s' to inventory."), *ItemUID.ToString()));
		return;
	}

	if (InventoryComponent->EquipItem(ItemUID) == false)
	{
		SendCheatMessage(this, FString::Printf(TEXT("EquipItem failed: could not equip '%s'."), *ItemUID.ToString()));
		return;
	}

	SendCheatMessage(
		this,
		FString::Printf(
			TEXT("Equipped [%s] %s"),
			*GetEquipmentTypeDisplayName(EquipmentData->EquipmentType),
			*ItemUID.ToString()));
}

void UMKCheatManager::ListEquippedItems()
{
	UInventoryComponent* InventoryComponent = GetPlayerInventoryComponent(this);
	if (::IsValid(InventoryComponent) == false)
	{
		SendCheatMessage(this, TEXT("ListEquippedItems failed: InventoryComponent is invalid."));
		return;
	}

	SendCheatMessage(this, TEXT("Equipped items:"));

	for (const EEuipmentType EquipmentType : GetCheatSupportedEquipmentTypes())
	{
		const FName EquippedItemKey = InventoryComponent->GetEquippedItem(EquipmentType);
		if (EquippedItemKey.IsNone())
		{
			SendCheatMessage(
				this,
				FString::Printf(TEXT("%s: None"), *GetEquipmentTypeDisplayName(EquipmentType)));
			continue;
		}

		const FEquipmentItemDataTableRow* EquipmentData = FEquipmentItemDataTableUtil::FindEquipmentItemData(this, EquippedItemKey);
		const FString EquipmentDisplayName = EquipmentData != nullptr
			? EquipmentData->EquipmentName.ToString()
			: FString();
		const FString EquipmentName = EquipmentDisplayName.IsEmpty() == false
			? EquipmentDisplayName
			: EquippedItemKey.ToString();

		SendCheatMessage(
			this,
			FString::Printf(
				TEXT("%s: %s (%s)"),
				*GetEquipmentTypeDisplayName(EquipmentType),
				*EquippedItemKey.ToString(),
				*EquipmentName));
	}
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

void UMKCheatManager::ToggleInventoryWidget()
{
	AMKPlayerController* PlayerController = Cast<AMKPlayerController>(UGameplayStatics::GetPlayerController(this, 0));
	if (::IsValid(PlayerController) == false)
	{
		return;
	}

	PlayerController->ToggleInventoryWidget();
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
