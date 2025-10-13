// LINK

#include "ProjectMK/Component/InteractComponent.h"

#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Interface/Interactable.h"
#include "ProjectMK/System/Enums/GlobalEnums.h"

UInteractComponent::UInteractComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.bStartWithTickEnabled = true;
}

bool UInteractComponent::TryInteract()
{
	IInteractable* InteractableActor = Cast<IInteractable>(InteractingActor);
	if (!InteractableActor)
	{
		return false;
	}

	InteractableActor->TryInteract(GetOwner());
	InteractingTime += GetWorld()->GetDeltaSeconds();
	UpdateInteractPosition();
	
	return true;
}

void UInteractComponent::UpdateCharacterDirection(const FVector& NewDir)
{
	AActor* Owner = GetOwner();
	if (::IsValid(Owner) == false)
	{
		return;
	}

	InteractDir = NewDir;

	FVector Start = Owner->GetActorLocation();
	FVector End = Start + NewDir * InteractDistance;

	TArray<FHitResult> Hits;
	FCollisionQueryParams Params;
	Params.AddIgnoredActor(GetOwner());

	DrawDebugLine(GetWorld(), Start, End, FColor::Red, false);

	AActor* NewInteractingActor = nullptr;
	if (GetWorld()->LineTraceMultiByChannel(Hits, Start, End, ECC_Visibility, Params))
	{
		for (const auto& Hit : Hits)
		{
			IInteractable* InteractableActor = Cast<IInteractable>(Hit.GetActor());
			if (InteractableActor == nullptr)
			{
				continue;
			}
			NewInteractingActor = Hit.GetActor();
			break;
		}
	}

	if (NewInteractingActor != InteractingActor)
	{
		if (InteractingActor.IsValid())
		{
			IInteractable* InteractableActor = Cast<IInteractable>(InteractingActor);
			if (InteractableActor)
			{
				InteractableActor->EndInteract();
			}
		}

		InteractingActor = NewInteractingActor;

		if (InteractingActor.IsValid())
		{
			InteractStartPoint = GetOwner()->GetActorLocation();
		}
		else
		{
			InteractStartPoint = FVector::ZeroVector;
		}

		InteractingTime = 0.f;
	}

	TryInteract();
}

void UInteractComponent::UpdateInteractPosition()
{
	if (::IsValid(GetOwner()) == false)
	{
		return;
	}

	if (InteractingActor.IsValid() == false)
	{
		return;
	}

	//TODO: 하드코딩 수정
	int32 BlockSize = 16;

	const FVector& InteractingActorLocation = InteractingActor->GetActorLocation();
	FVector TargetLocation = InteractingActorLocation - InteractDir * BlockSize;

	if (GetOwner()->GetActorLocation() == TargetLocation)
	{
		return;
	}

	float Alpha = FMath::Clamp(InteractingTime / InteractPositionMoveDuration, 0.0f, 1.0f);
	FVector NewLocation = FMath::Lerp(InteractStartPoint, TargetLocation, Alpha);
	GetOwner()->SetActorLocation(NewLocation);
}
