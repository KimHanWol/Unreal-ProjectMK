#include "ProjectMK/Actor/Shop/ShopActor.h"

#include "Components/BoxComponent.h"
#include "GameFramework/Pawn.h"
#include "Kismet/GameplayStatics.h"
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
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
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

void AShopActor::NotifyActorOnClicked(FKey ButtonPressed)
{
	Super::NotifyActorOnClicked(ButtonPressed);

	TryInteract(UGameplayStatics::GetPlayerPawn(this, 0));
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
}

void AShopActor::OnOwnerDestroyed(AActor* DestroyedActor)
{
	Destroy();
}
