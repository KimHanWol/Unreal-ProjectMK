// LINK

#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "MKPlayerController.generated.h"

class UHUDWidget;

UCLASS()
class PROJECTMK_API AMKPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AMKPlayerController();

	void ToggleShopTestWidget();
};
