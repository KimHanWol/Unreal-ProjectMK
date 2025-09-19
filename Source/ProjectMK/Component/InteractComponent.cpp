// LINK

#include "ProjectMK/Component/InteractComponent.h"

#include "ProjectMK/Interface/Interactable.h"

UInteractComponent::UInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

bool UInteractComponent::TryInteract()
{
	if (InteractingActor.IsValid() == false)
	{
		return false;
	}

	IInteractable* InteractableActor = Cast<IInteractable>(InteractingActor);
	if (InteractableActor)
	{
		return InteractableActor->TryInteract(GetOwner());
	}

	return false;
}

void UInteractComponent::UpdateCharacterDirection(const FVector& NewDir)
{
	if (NewDir == FVector::ZeroVector)
	{
		return;
	}

	AActor* Owner = GetOwner();
	if (::IsValid(Owner) == false)
	{
		return;
	}

	FVector Start = Owner->GetActorLocation();
	FVector End = Start + NewDir * InteractDistance;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceMultiByChannel(Hits, Start, End, ECC_Visibility, Params))
	{
		for (const auto& Hit : Hits)
		{
			IInteractable* InteractableActor = Cast<IInteractable>(Hit.GetActor());
			if (InteractableActor)
			{
				InteractingActor = Hit.GetActor();
				TryInteract();
				break;
			}
		}
	}
}