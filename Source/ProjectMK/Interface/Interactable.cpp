// LINK

#include "ProjectMK/Interface/Interactable.h"

#include "AbilitySystemComponent.h"
#include "GameFramework/Character.h"

bool IInteractable::CanInteract(AActor* Interactor)
{
	if (::IsValid(Interactor) == false)
	{
		return false;
	}

	ACharacter* InteractingCharacter = Cast<ACharacter>(Interactor);
	if (::IsValid(InteractingCharacter) == false)
	{
		return false;
	}

	if (FMath::IsNearlyZero(InteractingCharacter->GetVelocity().Z) == false)
	{
		return false;
	}

	return true;
}

bool IInteractable::TryInteract(AActor* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return false;
	}

	UAbilitySystemComponent* InteractorASC = Interactor->GetComponentByClass<UAbilitySystemComponent>();
	if (::IsValid(InteractorASC) == false)
	{
		return false;
	}

	const FGameplayTag& InteractEventTag = GetInteractEventTag();
	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = Cast<AActor>(this);
	EventData.EventTag = InteractEventTag;

	return InteractorASC->HandleGameplayEvent(EventData.EventTag, &EventData) > 0; 
}
