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

void UInteractComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

	CheckInteractableActor();
}

void UInteractComponent::CheckInteractableActor()
{
	AActor* Owner = GetOwner();
	if (::IsValid(Owner) == false)
	{
		return;
	}

	FVector ForwardDir = Owner->GetActorForwardVector(); // 또는 입력 방향
	FVector Start = Owner->GetActorLocation();
	FVector End = Start + ForwardDir * InteractDistance; // 블록 탐지 거리

	FHitResult Hit;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		IInteractable* InteractableActor = Cast<IInteractable>(Hit.GetActor());
		if (InteractableActor)
		{
			InteractingActor = Hit.GetActor();
		}
	}
}
