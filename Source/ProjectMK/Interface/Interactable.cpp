// LINK

#include "AbilitySystemComponent.h"
#include "ProjectMK/Interface/Interactable.h"

bool IInteractable::CanInteract(AActor* Interactor)
{
	if (::IsValid(Interactor) == false)
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