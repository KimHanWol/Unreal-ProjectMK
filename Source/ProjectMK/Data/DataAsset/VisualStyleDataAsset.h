#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Styling/SlateBrush.h"
#include "VisualStyleDataAsset.generated.h"

class UPaperSprite;
class UPaperFlipbook;
class UMaterialInterface;

UCLASS(BlueprintType)
class PROJECTMK_API UVisualStyleDataAsset : public UDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
	TMap<FName, FSlateBrush> Brushes;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TObjectPtr<UPaperSprite> CushionOverlay;

	/** Index bits: Left=1, Right=2, Top=4, Bottom=8. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "World")
	TArray<TObjectPtr<UMaterialInterface>> TerrainEdgeMaterials;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Effects")
	TMap<FName, TObjectPtr<UPaperFlipbook>> Effects;
};
