// LINK

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CheatManager.h"
#include "MKCheatManager.generated.h"

UCLASS()
class PROJECTMK_API UMKCheatManager : public UCheatManager
{
	GENERATED_BODY()

public:
	UFUNCTION(Exec)
	void DamagePlayer(float Damage = 1.f);

	UFUNCTION(Exec)
	void DrainOxygen(float OxygenAmount = 1.f);

	UFUNCTION(Exec)
	void GiveItem(FName ItemUID, int32 ItemCount = 1);

	UFUNCTION(Exec)
	void GetItem(FName ItemUID = NAME_None, int32 ItemCount = 1);

	UFUNCTION(Exec)
	void ResetMineableLevel(int32 GenerationSeed = -1);

	UFUNCTION(Exec)
	void EnableBlockDebugNumbers();

	UFUNCTION(Exec)
	void SetSkillDebugLog(bool bEnabled = true);

	UFUNCTION(Exec)
	void ToggleSkillDebugLog();

	UFUNCTION(Exec)
	void ToggleInventoryWidget();

	UFUNCTION(Exec)
	void OpenShop();

	UFUNCTION(Exec)
	void UseQuickSlot(int32 SlotNumber = 1);

	UFUNCTION(Exec)
	void UnlockRecipe(FName RecipeKey);

	UFUNCTION(Exec)
	void CraftRecipe(FName RecipeKey);

private:
	class AMKCharacter* GetLocalPlayerCharacter() const;
	class AMKPlayerController* GetMKPlayerController() const;
};
