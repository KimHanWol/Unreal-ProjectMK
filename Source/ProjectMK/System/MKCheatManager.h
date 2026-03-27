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
	void GiveItem(FName ItemUID, int32 ItemCount = 1);

	UFUNCTION(Exec)
	void GetItem(FName ItemUID = NAME_None, int32 ItemCount = 1);

	UFUNCTION(Exec)
	void EquipItem(FName ItemUID = NAME_None);

	UFUNCTION(Exec)
	void ListEquippedItems();

	UFUNCTION(Exec)
	void EnableBlockDebugNumbers();

	UFUNCTION(Exec)
	void ToggleInventoryWidget();

	UFUNCTION(Exec)
	void ToggleShopTestWidget();
};
