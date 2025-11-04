// LINK

#include "ProjectMK/Helper/MKBlueprintFunctionLibrary.h"

#include "PaperSprite.h"

TSoftObjectPtr<UTexture2D> UMKBlueprintFunctionLibrary::ConvItemTextureFromPaperSprite(TSoftObjectPtr<UPaperSprite> TargetSprite)
{
	if (TargetSprite.IsNull())
	{
		return nullptr;
	}

	return TargetSprite.LoadSynchronous()->GetBakedTexture();
}
