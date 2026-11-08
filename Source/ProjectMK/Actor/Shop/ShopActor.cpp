#include "ProjectMK/Actor/Shop/ShopActor.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Controller/MKPlayerController.h"
#include "ProjectMK/Core/Manager/SoundManager.h"

AShopActor::AShopActor()
{
	PrimaryActorTick.bCanEverTick = false;

	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	RootComponent = InteractionBox;
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
	ShopSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("ShopSprite"));
	ShopSprite->SetupAttachment(RootComponent);
	ShopSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	ShopSprite->SetTranslucentSortPriority(-5);
}

bool AShopActor::CanInteract(AActor* Interactor)
{
	if (::IsValid(Interactor) == false || ::IsValid(InteractionBox) == false) { return false; }
	const FVector Local = InteractionBox->GetComponentTransform().InverseTransformPosition(Interactor->GetActorLocation());
	const FVector Extent = InteractionBox->GetUnscaledBoxExtent();
	return FMath::Abs(Local.X) <= Extent.X && FMath::Abs(Local.Z) <= Extent.Z && FMath::Abs(Local.Y) <= Extent.Y;
}

void AShopActor::BeginPlay()
{
	Super::BeginPlay();

	BindEvents();
}

void AShopActor::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
	UnbindEvents();

	Super::EndPlay(EndPlayReason);
}

float AShopActor::GetInteractRange() const
{
	return InteractRange;
}

bool AShopActor::OnInteract(AActor* Interactor)
{
	const APawn* InteractorPawn = Cast<APawn>(Interactor);
	AMKPlayerController* PlayerController = ::IsValid(InteractorPawn) ? Cast<AMKPlayerController>(InteractorPawn->GetController()) : nullptr;
	if (::IsValid(PlayerController) == false || PlayerController->TryOpenShop() == false)
	{
		return false;
	}

	USoundManager::TryPlaySFX(this, ESFXType::Shop_Open);
	return true;
}

void AShopActor::BindEvents()
{
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->OnDestroyed.AddUniqueDynamic(this, &AShopActor::OnOwnerDestroyed);
	}
}

void AShopActor::UnbindEvents()
{
	if (AActor* OwnerActor = GetOwner())
	{
		OwnerActor->OnDestroyed.RemoveDynamic(this, &AShopActor::OnOwnerDestroyed);
	}
}

void AShopActor::InitializeShop(const FIntPoint& TileSize)
{
	if (::IsValid(InteractionBox) == false)
	{
		return;
	}

	const UDataManager* DataManager = UDataManager::Get(this);
	const FBlockDataTableRow* ShopData = ::IsValid(DataManager) ? DataManager->GetBlockDataTableRow(FName(TEXT("Shop"))) : nullptr;
	UPaperSprite* Sprite = ShopData != nullptr ? ShopData->TileSprite.LoadSynchronous() : nullptr;
	if (::IsValid(Sprite))
	{
		ShopSprite->SetSprite(Sprite);
		const FVector2D SourceSize = Sprite->GetSourceSize() / Sprite->GetPixelsPerUnrealUnit();
		const float Scale = FMath::Min(TileSize.X / FMath::Max(1.f, SourceSize.X), TileSize.Y / FMath::Max(1.f, SourceSize.Y)) * VisualSizeInTiles;
		const float VisualHeight = SourceSize.Y * Scale;
		const float BaseZ = GetActorLocation().Z - TileSize.Y * 0.5f;
		FHitResult GroundHit;
		FCollisionQueryParams Params;
		Params.AddIgnoredActor(this);
		Params.AddIgnoredActor(GetOwner());
		const FVector Start = GetActorLocation() + FVector(0.f, 0.f, TileSize.Y);
		const FVector End = Start - FVector(0.f, 0.f, TileSize.Y * 12.f);
		const float GroundZ = GetWorld()->LineTraceSingleByChannel(GroundHit, Start, End, ECC_Visibility, Params) ? GroundHit.ImpactPoint.Z : BaseZ;
		SetActorLocation(FVector(GetActorLocation().X, GetActorLocation().Y, GroundZ + VisualHeight * 0.5f));
		InteractionBox->SetBoxExtent(FVector(SourceSize.X * Scale * 0.5f, 32.f, VisualHeight * 0.5f));
		ShopSprite->SetRelativeScale3D(FVector(Scale, 1.f, Scale));
		ShopSprite->SetRelativeLocation(FVector(0.f, -12.f, 0.f));
		if (AActor* OwnerBlock = GetOwner())
		{
			OwnerBlock->SetActorHiddenInGame(true);
		}
	}
}

void AShopActor::OnOwnerDestroyed(AActor* DestroyedActor)
{
	Destroy();
}
