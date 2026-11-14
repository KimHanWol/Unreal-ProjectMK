#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ProjectMK/Data/MineTownData.h"
#include "TownProgressSubsystem.generated.h"

class AMKCharacter;
class AMineFacilityActor;
class UMKSaveGame;

UCLASS()
class PROJECTMK_API UTownProgressSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()
public:
	virtual void Tick(float DeltaSeconds) override;
	virtual TStatId GetStatId() const override;
	void GenerateFacilities();
	void ClearFacilities();
	void WriteSaveData(UMKSaveGame* Save) const;
	void RestoreProgress(const UMKSaveGame* Save);
	void ClearQuestCooldownsForPlaytest();
	bool IsSurfaceSiteReserved(int32 Column, int32 HalfWidth) const;
	bool IsRailFactoryUnlocked() const;
	bool CanAccessRailFactory(const AActor* Player) const;
	bool CanTravelToDepth(int32 Depth) const;
	bool TryTravel(AMKCharacter* Player, AMineFacilityActor* Source, int32 Depth);
	bool TryQuestAction(AMKCharacter* Player, AMineFacilityActor* Resident);
	int32 GetMaximumDepth() const;
	bool HasRelic(FName Key) const;
	FText GetDialogue(const AMineFacilityActor* Facility, AMKCharacter* Player) const;
	FText GetQuestActionLabel(const AMineFacilityActor* Facility) const;
	bool CanUseQuestAction(const AMineFacilityActor* Facility, AMKCharacter* Player) const;
	const TArray<TWeakObjectPtr<AMineFacilityActor>>& GetFacilities() const;
	DECLARE_MULTICAST_DELEGATE(FOnTownProgressChanged);
	FOnTownProgressChanged OnTownProgressChanged;
private:
	void LoadProgress();
	const FTownQuestDefinition* GetQuestDefinition(int32 Resident) const;
	TArray<TWeakObjectPtr<AMineFacilityActor>> Facilities;
	TArray<FTownQuestState> Quests;
	TSet<FName> OwnedRelics;
	int32 MaximumDepth = 0;
	float UpdateTime = 0.f;
	float LastSavedAt = 0.f;
	bool bLoaded = false;
	bool bProgressDirty = false;
};
