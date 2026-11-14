#include "ProjectMK/Actor/Transport/MineCartActor.h"

#include "Components/BoxComponent.h"
#include "PaperSpriteComponent.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Actor/Transport/RailTrackActor.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"

AMineCartActor::AMineCartActor()
{
	PrimaryActorTick.bCanEverTick = false;
	CartBody = CreateDefaultSubobject<UBoxComponent>(TEXT("CartBody"));
	RootComponent = CartBody;
	CartBody->SetBoxExtent(FVector(10.f, 8.f, 5.f));
	CartBody->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	CartBody->SetCollisionResponseToAllChannels(ECR_Ignore);
	CartBody->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
	CartBody->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
	CartBody->SetCanEverAffectNavigation(false);
	CartSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Cart"));
	CartSprite->SetupAttachment(RootComponent);
	CartSprite->SetRelativeLocation(FVector(0.f, -10.f, 2.f));
	CartSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CartSprite->SetTranslucentSortPriority(-4);
}

bool AMineCartActor::CanInteract(AActor* Interactor)
{
	return Interactor != nullptr && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(30.f);
}

FName AMineCartActor::GetRecoveryItemKey() const { return CartData.bAutomatic ? TEXT("AutoMineCart") : TEXT("MineCart"); }

FText AMineCartActor::GetRecoveryBlockedReason(AActor* Interactor) const
{
	if (Cargo.IsValid()) { return NSLOCTEXT("Rail", "RemoveCargoFirst", "상자를 먼저 회수하세요"); }
	return IInteractable::GetRecoveryBlockedReason(Interactor);
}

void AMineCartActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Reason == EEndPlayReason::Destroyed) { GetWorld()->GetSubsystem<UMineTransportSubsystem>()->NotifyCartRemoved(this); }
	Super::EndPlay(Reason);
}

void AMineCartActor::InitializeCart(const FMineCartSaveData& Data, ARailTrackActor* Rail)
{
	CartData = Data; CurrentRail = Rail;
	const auto* Style = UDataManager::Get(this)->GetVisualStyleDataAsset();
	CartSprite->SetSprite(Style != nullptr ? Style->TownSprites.FindRef(GetRecoveryItemKey()) : nullptr);
	if (Rail != nullptr) { SetActorLocation(FMath::Lerp(Rail->GetStartWorld(), Rail->GetEndWorld(), FMath::Clamp(CartData.Position, 0.f, 1.f)) + FVector(0.f, 0.f, 5.5f)); }
}

void AMineCartActor::UpdateCart(float DeltaSeconds)
{
	ARailTrackActor* Rail = CurrentRail.Get();
	if (Rail == nullptr) { return; }
	UMineTransportSubsystem* Network = GetWorld()->GetSubsystem<UMineTransportSubsystem>();
	const FVector Tangent = (Rail->GetEndWorld() - Rail->GetStartWorld()).GetSafeNormal();
	AMKCharacter* Player = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	float Push = 0.f;
	if (Player != nullptr)
	{
		const FVector Offset = GetActorLocation() - Player->GetActorLocation();
		const float Input = Player->GetCharacterDirection().X;
		if (FMath::Abs(Offset.X) < 23.f && FMath::Abs(Offset.Z) < 16.f && Input * Offset.X > 0.5f) { Push = FMath::Sign(Input * Tangent.X); }
	}
	if (CartData.bAutomatic)
	{
		if (Push != 0.f) { CartData.DriveDirection = FMath::RoundToInt(Push); }
		CartData.Speed = FMath::FInterpConstantTo(CartData.Speed, CartData.DriveDirection * 24.f, DeltaSeconds, 30.f);
	}
	else
	{
		CartData.Speed += (-180.f * Tangent.Z + Push * 24.f) * DeltaSeconds;
		CartData.Speed = FMath::FInterpConstantTo(CartData.Speed, 0.f, DeltaSeconds, Push == 0.f ? 8.f : 3.f);
		CartData.Speed = FMath::Clamp(CartData.Speed, Push != 0.f ? -18.f : -65.f, Push != 0.f ? 18.f : 65.f);
	}
	float NextPosition = CartData.Position + CartData.Speed * DeltaSeconds / FVector::Distance(Rail->GetStartWorld(), Rail->GetEndWorld());
	if (NextPosition < 0.f || NextPosition > 1.f)
	{
		const bool bAtEnd = NextPosition > 1.f;
		const FIntPoint Exit = bAtEnd ? Rail->GetSaveData().End : Rail->GetSaveData().Start;
		ARailTrackActor* Next = Network->FindNextRail(Rail, Exit);
		if (Next == nullptr) { NextPosition = FMath::Clamp(NextPosition, 0.f, 1.f); CartData.Speed = 0.f; CartData.DriveDirection = 0; }
		else
		{
			const float Overshoot = FMath::Abs(NextPosition - (bAtEnd ? 1.f : 0.f)) * FVector::Distance(Rail->GetStartWorld(), Rail->GetEndWorld());
			const bool bForward = Next->GetSaveData().Start == Exit;
			NextPosition = bForward ? Overshoot / FVector::Distance(Next->GetStartWorld(), Next->GetEndWorld()) : 1.f - Overshoot / FVector::Distance(Next->GetStartWorld(), Next->GetEndWorld());
			CartData.Speed = FMath::Abs(CartData.Speed) * (bForward ? 1.f : -1.f);
			if (CartData.DriveDirection != 0) { CartData.DriveDirection = bForward ? 1 : -1; }
			Rail = Next;
		}
	}
	const FVector Position = FMath::Lerp(Rail->GetStartWorld(), Rail->GetEndWorld(), FMath::Clamp(NextPosition, 0.f, 1.f)) + FVector(0.f, 0.f, 5.5f);
	if (Network->CanCartOccupy(this, Position)) { CartData.Position = FMath::Clamp(NextPosition, 0.f, 1.f); CurrentRail = Rail; CartData.RailId = Rail->GetSaveData().Id; SetActorLocation(Position); }
	else { CartData.Speed = 0.f; CartData.DriveDirection = 0; }
}

void AMineCartActor::SetCargo(AStorageChestActor* Chest) { Cargo = Chest; }
AStorageChestActor* AMineCartActor::GetCargo() const { return Cargo.Get(); }
ARailTrackActor* AMineCartActor::GetRail() const { return CurrentRail.Get(); }
FGuid AMineCartActor::GetCartId() const { return CartData.Id; }
FMineCartSaveData AMineCartActor::GetSaveData() const { return CartData; }
float AMineCartActor::GetSpeed() const { return CartData.Speed; }
