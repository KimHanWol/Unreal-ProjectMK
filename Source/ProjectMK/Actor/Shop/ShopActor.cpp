#include "ProjectMK/Actor/Shop/ShopActor.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Controller/MKPlayerController.h"
#include "ProjectMK/Core/Manager/SoundManager.h"

namespace
{
	constexpr float ShopHalfWidthInTiles = 6.f;
	constexpr float ShopHalfHeightInTiles = 2.f;
	// 월드 상점은 플레이어를 막지 않고 클릭과 근접 검색에만 사용한다.
	constexpr float InteractionBoxDepth = 20.f;
}

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

	InteractionBox->SetBoxExtent(FVector(TileSize.X * ShopHalfWidthInTiles, InteractionBoxDepth, TileSize.Y * ShopHalfHeightInTiles));
	const UDataManager* DataManager = UDataManager::Get(this);
	const FBlockDataTableRow* ShopData = ::IsValid(DataManager) ? DataManager->GetBlockDataTableRow(FName(TEXT("Shop"))) : nullptr;
	UPaperSprite* Sprite = ShopData != nullptr ? ShopData->TileSprite.LoadSynchronous() : nullptr;
	if (::IsValid(Sprite))
	{
		ShopSprite->SetSprite(Sprite);
		const FVector2D SourceSize = Sprite->GetSourceSize() / Sprite->GetPixelsPerUnrealUnit();
		const float Scale = FMath::Min(TileSize.X / FMath::Max(1.f, SourceSize.X), TileSize.Y / FMath::Max(1.f, SourceSize.Y)) * VisualSizeInTiles;
		ShopSprite->SetRelativeScale3D(FVector(Scale, 1.f, Scale));
		ShopSprite->SetRelativeLocation(FVector(0.f, 0.f, (VisualSizeInTiles - 1.f) * TileSize.Y * 0.5f));
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
