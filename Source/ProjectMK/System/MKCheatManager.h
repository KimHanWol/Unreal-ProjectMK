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
	void EnableBlockDebugNumbers();

	UFUNCTION(Exec)
	void ToggleInventoryWidget();

	UFUNCTION(Exec)
	void ToggleShopTestWidget();
};
