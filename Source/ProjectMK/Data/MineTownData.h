#pragma once

#include "CoreMinimal.h"
#include "MineTownData.generated.h"

UENUM(BlueprintType)
enum class EMineFacilityKind : uint8
{
	Elevator,
	RailFactory,
	HouseA,
	HouseB,
};

USTRUCT(BlueprintType)
struct FRailSaveData
{
	GENERATED_BODY()
	UPROPERTY() FGuid Id;
	UPROPERTY() FIntPoint Start = FIntPoint::ZeroValue;
	UPROPERTY() FIntPoint End = FIntPoint::ZeroValue;
	UPROPERTY() bool bEndDisconnected = false;
};

USTRUCT(BlueprintType)
struct FMineCartSaveData
{
	GENERATED_BODY()
	UPROPERTY() FGuid Id;
	UPROPERTY() FGuid RailId;
	UPROPERTY() float Position = 0.5f;
	UPROPERTY() float Speed = 0.f;
	UPROPERTY() bool bAutomatic = false;
	UPROPERTY() int32 DriveDirection = 0;
};

USTRUCT(BlueprintType)
struct FTownQuestState
{
	GENERATED_BODY()
	UPROPERTY() bool bActive = false;
	UPROPERTY() int32 CompletedCount = 0;
	UPROPERTY() int64 NextOfferAt = 0;
};

USTRUCT(BlueprintType)
struct FTownQuestDefinition
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FText Title;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) FName RequiredItem = TEXT("Ore_Coal");
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RequiredCount = 8;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RewardCoins = 30;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 MinimumDepth = 0;
};

USTRUCT(BlueprintType)
struct FTownFacilitySettings
{
	GENERATED_BODY()
	UPROPERTY(EditAnywhere, BlueprintReadOnly) bool bEnabled = true;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<int32> ElevatorDepths = { 32, 72 };
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RailFactoryUnlockDepth = 48;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HouseAColumn = 15;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 HouseBColumn = 39;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 ElevatorColumn = 94;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 RailFactoryColumn = 106;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0")) int32 QuestCooldownSeconds = 180;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, meta = (ClampMin = "0", ClampMax = "1")) float QuestRelicChance = 0.05f;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) int32 GuaranteedRelicQuestCount = 3;
	UPROPERTY(EditAnywhere, BlueprintReadOnly) TArray<FTownQuestDefinition> Quests;
};

USTRUCT(BlueprintType)
struct FInstalledStructureSaveData
{
	GENERATED_BODY()
	UPROPERTY() FName ItemKey = NAME_None;
	UPROPERTY() FVector Location = FVector::ZeroVector;
};
