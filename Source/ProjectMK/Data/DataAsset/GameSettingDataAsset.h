#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Data/DataAsset/MKDataAssetBase.h"
#include "ProjectMK/Data/DataTable/EquipmentItemDataTableRow.h"

#include "GameSettingDataAsset.generated.h"

USTRUCT(BlueprintType)
struct FEquipmentOverlayZOrderEntry
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    EEuipmentType EquipmentType = EEuipmentType::Halmet;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    int32 ZOrder = 0;
};

UCLASS(BlueprintType)
class PROJECTMK_API UGameSettingDataAsset : public UMKDataAssetBase
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintPure, Category = "Equipment")
    int32 GetEquipmentOverlayZOrder(EEuipmentType EquipmentType) const;

    // The surface block Y position where oxygen is immediately restored to max.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oxygen")
    int32 SurfaceBlockPositionY = 0;

    // Oxygen loss increases by 1 each time the character goes this many blocks deeper than the surface.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oxygen", meta = (ClampMin = "0.0"))
    int32 DepthPerOxygenLoss = 1;

    // Interval in seconds used by the periodic oxygen drain gameplay effect.
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Oxygen", meta = (ClampMin = "0.01"))
    float OxygenDrainTickInterval = 0.1f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Falling Block")
    int32 FallingBlockDamagePerBlock = 1;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Block", meta = (ClampMin = "0.01"))
    float BlockItemOverlaySpriteScale = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Item", meta = (ClampMin = "0.01"))
    float WorldItemSpriteScale = 0.7f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Character")
    bool bReverseOverrideVisualFacingDirection = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Equipment")
    TArray<FEquipmentOverlayZOrderEntry> EquipmentOverlayZOrders;
};
