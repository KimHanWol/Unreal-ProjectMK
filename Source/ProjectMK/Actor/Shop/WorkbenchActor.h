#pragma once

#include "CoreMinimal.h"
#include "ProjectMK/Actor/Shop/ShopActor.h"
#include "WorkbenchActor.generated.h"

class UPaperSprite;

UCLASS()
class PROJECTMK_API AWorkbenchActor : public AShopActor
{
	GENERATED_BODY()

public:
	void InitializeWorkbench(UPaperSprite* Sprite);

protected:
	virtual bool OnInteract(AActor* Interactor) override;
};
