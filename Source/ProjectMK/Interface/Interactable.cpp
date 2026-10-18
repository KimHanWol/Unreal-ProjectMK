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

	return IsInInteractRange(Interactor);
}

bool IInteractable::TryInteract(AActor* Interactor)
{
	if (!CanInteract(Interactor))
	{
		return false;
	}

	const FGameplayTag& InteractEventTag = GetInteractEventTag();
	if (InteractEventTag.IsValid() == false)
	{
		return OnInteract(Interactor);
	}

	UAbilitySystemComponent* InteractorASC = Interactor->GetComponentByClass<UAbilitySystemComponent>();
	if (::IsValid(InteractorASC) == false)
	{
		return false;
	}

	FGameplayEventData EventData;
	EventData.Instigator = Interactor;
	EventData.Target = Cast<AActor>(this);
	EventData.EventTag = InteractEventTag;

	return InteractorASC->HandleGameplayEvent(EventData.EventTag, &EventData) > 0;
}

bool IInteractable::IsInInteractRange(const AActor* Interactor) const
{
	const float InteractRange = GetInteractRange();
	if (InteractRange <= 0.f)
	{
		return true;
	}

	const AActor* InteractableActor = Cast<AActor>(const_cast<IInteractable*>(this));
	if (::IsValid(InteractableActor) == false || ::IsValid(Interactor) == false)
	{
		return false;
	}

	return FVector::DistSquared(InteractableActor->GetActorLocation(), Interactor->GetActorLocation()) <= FMath::Square(InteractRange);
}
