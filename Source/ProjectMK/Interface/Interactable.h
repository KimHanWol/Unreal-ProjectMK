#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"
#include "UObject/Interface.h"

#include "Interactable.generated.h"

UINTERFACE(Blueprintable)
class PROJECTMK_API UInteractable : public UInterface
{
	GENERATED_BODY()
};

class IInteractable
{
	GENERATED_BODY()

public:
	virtual bool CanInteract(AActor* Interactor);
	virtual bool TryInteract(AActor* Interactor);
	virtual void EndInteract() {}

protected:
	/** 0 이하이면 거리 제한 없이 상호작용한다 */
	virtual float GetInteractRange() const { return 0.f; }

	/** 유효한 태그를 반환하면 상호작용을 Interactor 의 GAS 이벤트로 전달한다 */
	virtual const FGameplayTag GetInteractEventTag() { return FGameplayTag(); }

	/** 이벤트 태그가 없을 때 액터가 직접 처리하는 상호작용 */
	virtual bool OnInteract(AActor* Interactor) { return false; }

	bool IsInInteractRange(const AActor* Interactor) const;
};
