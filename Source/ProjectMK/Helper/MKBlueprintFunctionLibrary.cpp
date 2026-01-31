// LINK

#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"

#include "PaperSprite.h"
#include "ProjectMK/Actor/Block/BlockBase.h"

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
	return FVector2D((int32)(WorldPosition.X / 16), -(int32)(WorldPosition.Z / 16));
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
    FVector ResultVector;
    int32 LastX = (int32)TargetVector.X % 16;
    if (LastX > 8)
    {
        ResultVector.X = TargetVector.X - LastX + 16;
    }
    else
    {
        ResultVector.X = TargetVector.X - LastX;
    }

    int32 LastZ = (int32)TargetVector.Z % 16;
    if (LastZ > 8)
    {
        ResultVector.Z = TargetVector.Z - LastZ + 16;
    }
    else
    {
        ResultVector.Z = TargetVector.Z - LastZ;
    }

    return ResultVector;
}