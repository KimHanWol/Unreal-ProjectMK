#include "ProjectMK/Actor/Shop/WorkbenchActor.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Controller/MKPlayerController.h"

void AWorkbenchActor::InitializeWorkbench(UPaperSprite* Sprite)
{
	ShopSprite->SetSprite(Sprite);
	const FVector2D Source = Sprite->GetSourceSize();
	const float Scale = 28.f / FMath::Max(Source.X, Source.Y);
	ShopSprite->SetRelativeScale3D(FVector(Scale * Sprite->GetPixelsPerUnrealUnit(), 1.f, Scale * Sprite->GetPixelsPerUnrealUnit()));
	ShopSprite->SetRelativeLocation(FVector(0.f, -12.f, -8.f + Source.Y * Scale * 0.5f));
	InteractionBox->SetBoxExtent(FVector(32.f, 32.f, 24.f));
}

bool AWorkbenchActor::OnInteract(AActor* Interactor)
{
	const APawn* Pawn = Cast<APawn>(Interactor);
	AMKPlayerController* Controller = Pawn != nullptr ? Cast<AMKPlayerController>(Pawn->GetController()) : nullptr;
	if (::IsValid(Controller) == false) { return false; }
	Controller->ToggleCrafting();
	return true;
}
