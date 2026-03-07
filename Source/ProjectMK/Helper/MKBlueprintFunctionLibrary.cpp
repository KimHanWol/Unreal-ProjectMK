// LINK

#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"

#include "PaperSprite.h"
#include "ProjectMK/Actor/Block/BlockBase.h"
#include "ProjectMK/System/GlobalConstants.h"

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
