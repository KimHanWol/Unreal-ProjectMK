#include "ProjectMK/Helper/Utils/EquipmentItemDataTableUtil.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "Engine/Texture2D.h"
#include "Modules/ModuleManager.h"
#include "PaperSprite.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"
#include "ProjectMK/Helper/MKRuntimePaperSprite.h"

namespace
{
	constexpr int32 EquipmentPreviewAtlasCellSize = 256;

	TArray<FAssetData> GetEquipmentItemDataTableAssetListForLookup()
	{
		FAssetRegistryModule& AssetRegistryModule = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));

		FARFilter Filter;
		Filter.ClassPaths.Add(UDataTable::StaticClass()->GetClassPathName());
		Filter.bRecursiveClasses = true;

		TArray<FAssetData> DataTableAssets;
		AssetRegistryModule.Get().GetAssets(Filter, DataTableAssets);
		return DataTableAssets;
	}

	FIntPoint ResolveIdlePreviewCellSize(const UTexture2D* Texture)
	{
		if (::IsValid(Texture) == false)
		{
			return FIntPoint::ZeroValue;
		}

		return FIntPoint(
			FMath::Min(Texture->GetSizeX(), EquipmentPreviewAtlasCellSize),
			FMath::Min(Texture->GetSizeY(), EquipmentPreviewAtlasCellSize));
	}
}

const FEquipmentItemDataTableRow* FEquipmentItemDataTableUtil::FindEquipmentItemData(UObject* WorldContextObject, FName EquipmentKey)
{
	if (EquipmentKey.IsNone())
	{
		return nullptr;
	}

	const UDataManager* DataManager = UDataManager::Get(WorldContextObject);
	if (::IsValid(DataManager))
	{
		if (const FEquipmentItemDataTableRow* EquipmentData = DataManager->GetDataTableRow<FEquipmentItemDataTableRow>(EDataTableType::EquipmentItem, EquipmentKey))
		{
			return EquipmentData;
		}
	}

	const TArray<FAssetData> DataTableAssets = GetEquipmentItemDataTableAssetListForLookup();
	for (const FAssetData& DataTableAsset : DataTableAssets)
	{
		const FString RowStructPath = DataTableAsset.TagsAndValues.FindTag(TEXT("RowStructure")).AsString();
		if (RowStructPath.Contains(TEXT("EquipmentItemDataTableRow")) == false)
		{
			continue;
		}

		UDataTable* DataTable = Cast<UDataTable>(DataTableAsset.GetAsset());
		if (::IsValid(DataTable) == false || DataTable->GetRowStruct() != FEquipmentItemDataTableRow::StaticStruct())
		{
			continue;
		}

		if (const FEquipmentItemDataTableRow* EquipmentData = DataTable->FindRow<FEquipmentItemDataTableRow>(EquipmentKey, TEXT("FindEquipmentItemDataFallback")))
		{
			return EquipmentData;
		}
	}

	return nullptr;
}

TArray<FName> FEquipmentItemDataTableUtil::GetEquipmentItemRowNames(UObject* WorldContextObject)
{
	TSet<FName> UniqueRowNames;

	const UDataManager* DataManager = UDataManager::Get(WorldContextObject);
	if (::IsValid(DataManager))
	{
		if (UDataTable* EquipmentDataTable = DataManager->GetDataTable(EDataTableType::EquipmentItem))
		{
			for (const FName& RowName : EquipmentDataTable->GetRowNames())
			{
				UniqueRowNames.Add(RowName);
			}
		}
	}

	const TArray<FAssetData> DataTableAssets = GetEquipmentItemDataTableAssetListForLookup();
	for (const FAssetData& DataTableAsset : DataTableAssets)
	{
		const FString RowStructPath = DataTableAsset.TagsAndValues.FindTag(TEXT("RowStructure")).AsString();
		if (RowStructPath.Contains(TEXT("EquipmentItemDataTableRow")) == false)
		{
			continue;
		}

		const UDataTable* DataTable = Cast<UDataTable>(DataTableAsset.GetAsset());
		if (::IsValid(DataTable) == false || DataTable->GetRowStruct() != FEquipmentItemDataTableRow::StaticStruct())
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

UTexture2D* FEquipmentItemDataTableUtil::LoadIdlePreviewTexture(const FEquipmentItemDataTableRow& EquipmentData)
{
	const TSoftObjectPtr<UTexture2D>* IdleTexturePtr = EquipmentData.AnimationOverlayTextures.FindTexture(ECharacterAnimationType::Idle);
	if (IdleTexturePtr == nullptr || IdleTexturePtr->IsNull())
	{
		return nullptr;
	}

	return IdleTexturePtr->LoadSynchronous();
}

bool FEquipmentItemDataTableUtil::GetIdlePreviewTextureRegion(const FEquipmentItemDataTableRow& EquipmentData, UTexture2D*& OutTexture, FVector2D& OutSourceUV, FVector2D& OutSourceSize)
{
	OutTexture = LoadIdlePreviewTexture(EquipmentData);
	if (::IsValid(OutTexture) == false)
	{
		return false;
	}

	const FIntPoint CellSize = ResolveIdlePreviewCellSize(OutTexture);
	if (CellSize.X <= 0 || CellSize.Y <= 0)
	{
		OutTexture = nullptr;
		return false;
	}

	OutSourceUV = FVector2D::ZeroVector;
	OutSourceSize = FVector2D(static_cast<float>(CellSize.X), static_cast<float>(CellSize.Y));
	return true;
}

UPaperSprite* FEquipmentItemDataTableUtil::CreateIdlePreviewSprite(UObject* Outer, const FEquipmentItemDataTableRow& EquipmentData, float DefaultPixelsPerUnrealUnit)
{
	if (::IsValid(Outer) == false)
	{
		return nullptr;
	}

	UTexture2D* PreviewTexture = nullptr;
	FVector2D SourceUV = FVector2D::ZeroVector;
	FVector2D SourceSize = FVector2D::ZeroVector;
	if (GetIdlePreviewTextureRegion(EquipmentData, PreviewTexture, SourceUV, SourceSize) == false)
	{
		return nullptr;
	}

	UMKRuntimePaperSprite* RuntimeSprite = NewObject<UMKRuntimePaperSprite>(Outer);
	if (::IsValid(RuntimeSprite) == false)
	{
		return nullptr;
	}

	RuntimeSprite->InitializeFromAtlasCell(
		PreviewTexture,
		FIntPoint(FMath::RoundToInt(SourceUV.X), FMath::RoundToInt(SourceUV.Y)),
		FIntPoint(FMath::RoundToInt(SourceSize.X), FMath::RoundToInt(SourceSize.Y)),
		FMath::Max(DefaultPixelsPerUnrealUnit, KINDA_SMALL_NUMBER));

	return RuntimeSprite;
}
