// LINK

#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"

#include "AssetRegistry/AssetData.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "Engine/DataTable.h"
#include "Modules/ModuleManager.h"
#include "PaperSprite.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/Data/DataTable/CharacterDataTableRow.h"
#include "ProjectMK/Data/DataTable/ItemDataTableRow.h"
#include "ProjectMK/System/GlobalConstants.h"

TArray<FString> UMKBlueprintFunctionLibrary::GetItemRowNames()
{
	TArray<FString> ItemRowNames;

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
		if (RowStructPath.Contains(TEXT("ItemDataTableRow")) == false)
		{
			continue;
		}

		const UDataTable* DataTable = Cast<UDataTable>(DataTableAsset.GetAsset());
		if (::IsValid(DataTable) == false || DataTable->GetRowStruct() != FItemDataTableRow::StaticStruct())
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

	for (const FName& RowName : SortedRowNames)
	{
		ItemRowNames.Add(RowName.ToString());
	}

	return ItemRowNames;
}

TArray<FString> UMKBlueprintFunctionLibrary::GetCharacterRowNames()
{
	TArray<FString> CharacterRowNames;

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
		const bool bIsCharacterRowStruct =
			RowStructPath.Contains(TEXT("CharacterDataTableRow")) ||
			RowStructPath.Contains(TEXT("CharacterAnimationDataTableRow"));
		if (bIsCharacterRowStruct == false)
		{
			continue;
		}

		const UDataTable* DataTable = Cast<UDataTable>(DataTableAsset.GetAsset());
		const bool bIsSupportedCharacterDataTable =
			::IsValid(DataTable) &&
			(
				DataTable->GetRowStruct() == FCharacterDataTableRow::StaticStruct() ||
				DataTable->GetRowStruct() == FCharacterAnimationDataTableRow::StaticStruct()
			);
		if (bIsSupportedCharacterDataTable == false)
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

	for (const FName& RowName : SortedRowNames)
	{
		CharacterRowNames.Add(RowName.ToString());
	}

	return CharacterRowNames;
}

TSoftObjectPtr<UTexture2D> UMKBlueprintFunctionLibrary::ConvItemTextureFromPaperSprite(TSoftObjectPtr<UPaperSprite> TargetSprite)
{
	if (TargetSprite.IsNull())
	{
		return nullptr;
	}

	return TargetSprite.LoadSynchronous()->GetBakedTexture();
}

FVector2D UMKBlueprintFunctionLibrary::ConvertWorldPositionToBlockPosition(const FVector& WorldPosition)
{
	return FVector2D(
		FMath::FloorToInt(WorldPosition.X / BLOCK_SIZE),
		FMath::FloorToInt((-WorldPosition.Z) / BLOCK_SIZE)
	);
}

FVector2D UMKBlueprintFunctionLibrary::GetBlockPosition(ABlockBase* TargetBlock)
{
	if (::IsValid(TargetBlock))
	{
		return ConvertWorldPositionToBlockPosition(TargetBlock->GetActorLocation());
	}

	return FVector2D::Zero();
}

FVector UMKBlueprintFunctionLibrary::GetSnappingWorldPosition(const FVector& TargetVector)
{
	const FVector2D BlockPosition = ConvertWorldPositionToBlockPosition(TargetVector);

	return FVector(
		BlockPosition.X * BLOCK_SIZE,
		TargetVector.Y,
		-(BlockPosition.Y * BLOCK_SIZE)
	);
}
