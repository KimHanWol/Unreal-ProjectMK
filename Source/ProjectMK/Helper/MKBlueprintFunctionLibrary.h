// LINK

#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "MKBlueprintFunctionLibrary.generated.h"

class ABlockBase;
class UPaperSprite;

UCLASS(BlueprintType)
class PROJECTMK_API UMKBlueprintFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	UFUNCTION()
	static TArray<FString> GetItemRowNames();

	UFUNCTION()
	static TArray<FString> GetCharacterRowNames();

	UFUNCTION(BlueprintCallable)
	static TSoftObjectPtr<UTexture2D> ConvItemTextureFromPaperSprite(TSoftObjectPtr<UPaperSprite> TargetSprite);

	UFUNCTION(BlueprintCallable)
	static FVector2D ConvertWorldPositionToBlockPosition(const FVector& WorldPosition);

	UFUNCTION(BlueprintCallable)
	static FVector2D GetBlockPosition(ABlockBase* TargetBlock);

	UFUNCTION(BlueprintCallable)
	static FVector GetSnappingWorldPosition(const FVector& TargetVector);
};
