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
	virtual FName GetRecoveryItemKey() const override;
	void InitializeWorkbench(UPaperSprite* Sprite, FName ItemKey = TEXT("Workbench"));

protected:
	virtual bool OnInteract(AActor* Interactor) override;

private:
	FName WorkbenchItemKey = TEXT("Workbench");
};
