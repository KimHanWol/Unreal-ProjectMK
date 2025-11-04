//LINK

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MKBlueprintFunctionLibrary.generated.h"

class UPaperSprite;

UCLASS(BlueprintType)
class PROJECTMK_API UMKBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintCallable)
    static TSoftObjectPtr<UTexture2D> ConvItemTextureFromPaperSprite(TSoftObjectPtr<UPaperSprite> TargetSprite);
};
