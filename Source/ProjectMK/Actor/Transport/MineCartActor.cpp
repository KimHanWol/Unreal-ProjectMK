#include "ProjectMK/Actor/Transport/MineCartActor.h"

#include "Components/BoxComponent.h"
#include "PaperSpriteComponent.h"
#include "PaperSprite.h"
#include "Kismet/GameplayStatics.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Actor/Shop/StorageChestActor.h"
#include "ProjectMK/Actor/Transport/RailTrackActor.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
#include "ProjectMK/Data/DataAsset/GameSettingDataAsset.h"

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
	CartSprite->SetRelativeLocation(FVector(0.f, -6.f, 2.f));
	CartSprite->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	CartSprite->SetTranslucentSortPriority(-4);
}

bool AMineCartActor::CanInteract(AActor* Interactor)
{
	return Interactor != nullptr && FVector::DistSquared(Interactor->GetActorLocation(), GetActorLocation()) <= FMath::Square(40.f);
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
	if (Rail != nullptr)
	{
		const FVector Tangent = CalculateRailDirection(Rail,CartData.Position);
		SetActorLocationAndRotation(CalculateRailPosition(Rail, CartData.Position), FQuat(FVector::RightVector, -FMath::Atan2(Tangent.Z,Tangent.X)));
	}
}

void AMineCartActor::UpdateCart(float DeltaSeconds)
{
	ARailTrackActor* Rail = CurrentRail.Get();
	if (Rail == nullptr) { return; }
	UMineTransportSubsystem* Network = GetWorld()->GetSubsystem<UMineTransportSubsystem>();
	const auto& Settings = UDataManager::Get(this)->GetGameSettingDataAsset()->MineTransport;
	const FVector Tangent = CalculateRailDirection(Rail,CartData.Position);
	AMKCharacter* Player = Cast<AMKCharacter>(UGameplayStatics::GetPlayerCharacter(this, 0));
	float Push = 0.f;
	if (Player != nullptr)
	{
		const FVector Offset = GetActorLocation() - Player->GetActorLocation();
		const float Input = Player->GetCharacterDirection().X;
		if (FMath::Abs(Offset.X) < 25.f && FMath::Abs(Offset.Z) < 16.f && Input * Offset.X > 0.5f) { Push = FMath::Sign(Input * Tangent.X); }
	}
	PushTime = Push == 0.f ? 0.f : PushTime + DeltaSeconds;
	if (CartData.bAutomatic)
	{
		if (Push != 0.f) { CartData.DriveDirection = FMath::RoundToInt(Push); }
		CartData.Speed = FMath::FInterpConstantTo(CartData.Speed, CartData.DriveDirection * Settings.AutomaticSpeed, DeltaSeconds, 24.f);
	}
	else
	{
		const float Gravity = -Settings.GravityAcceleration * Tangent.Z;
		const float PushAcceleration = FMath::Lerp(Settings.PushInitialAcceleration, Settings.PushRunningAcceleration, FMath::Clamp(PushTime / Settings.PushWarmupSeconds, 0.f, 1.f));
		const float HillCompensation = FMath::Max(0.f, -Push * Gravity);
		CartData.Speed += (Gravity + Push * (PushAcceleration + HillCompensation)) * DeltaSeconds;
		CartData.Speed = FMath::FInterpConstantTo(CartData.Speed, 0.f, DeltaSeconds, Push == 0.f ? 8.f : 3.f);
		CartData.Speed = FMath::Clamp(CartData.Speed, -Settings.ManualTopSpeed, Settings.ManualTopSpeed);
	}
	float NextPosition = CartData.Position + CartData.Speed * DeltaSeconds / FVector::Distance(Rail->GetStartWorld(), Rail->GetEndWorld());
	// Carry all travelled distance through short pieces, including a reverse-facing adjoining piece.
	for (int32 Transition = 0; (NextPosition < 0.f || NextPosition > 1.f) && Transition < 32; ++Transition)
	{
		const bool bAtEnd = NextPosition > 1.f;
		const FIntPoint Exit = bAtEnd ? Rail->GetSaveData().End : Rail->GetSaveData().Start;
		ARailTrackActor* Next = Network->FindNextRail(Rail, Exit);
		if (Next == nullptr) { NextPosition = FMath::Clamp(NextPosition, 0.f, 1.f); CartData.Speed = 0.f; CartData.DriveDirection = 0; break; }
		const float Overshoot = FMath::Abs(NextPosition - (bAtEnd ? 1.f : 0.f)) * FVector::Distance(Rail->GetStartWorld(), Rail->GetEndWorld());
		const bool bForward = Next->GetSaveData().Start == Exit;
		const float Length = FVector::Distance(Next->GetStartWorld(), Next->GetEndWorld());
		NextPosition = bForward ? Overshoot / Length : 1.f - Overshoot / Length;
		CartData.Speed = FMath::Abs(CartData.Speed) * (bForward ? 1.f : -1.f);
		if (CartData.DriveDirection != 0) { CartData.DriveDirection = bForward ? 1 : -1; }
		Rail = Next;
	}
	const FVector Direction = CalculateRailDirection(Rail,NextPosition);
	const FQuat Rotation(FVector::RightVector,-FMath::Atan2(Direction.Z,Direction.X));
	const FVector Position = CalculateRailPosition(Rail,NextPosition);
	if (Network->CanCartOccupy(this, Position, Rotation))
	{
		CartData.Position = FMath::Clamp(NextPosition, 0.f, 1.f); CurrentRail = Rail; CartData.RailId = Rail->GetSaveData().Id;
		SetActorLocationAndRotation(Position,Rotation);
	}
	else { CartData.Speed = 0.f; PushTime = 0.f; }
}

void AMineCartActor::SetCargo(AStorageChestActor* Chest) { Cargo = Chest; }
AStorageChestActor* AMineCartActor::GetCargo() const { return Cargo.Get(); }
ARailTrackActor* AMineCartActor::GetRail() const { return CurrentRail.Get(); }
FGuid AMineCartActor::GetCartId() const { return CartData.Id; }
FMineCartSaveData AMineCartActor::GetSaveData() const { return CartData; }
float AMineCartActor::GetSpeed() const { return CartData.Speed; }

float AMineCartActor::GetCargoMountHeight() const
{
	const UPaperSprite* Sprite=CartSprite->GetSprite();
	// Native cart canvases retain three/four transparent pixels above their deck at 2px/unit.
	const float DeckInset = CartData.bAutomatic ? 2.f : 1.5f;
	return 10.f-DeckInset+(Sprite!=nullptr?Sprite->GetSourceSize().Y/Sprite->GetPixelsPerUnrealUnit()*0.5f:8.f);
}

FVector AMineCartActor::CalculateRailDirection(const ARailTrackActor* Rail, float Alpha)
{
	if (Rail == nullptr) { return FVector::ForwardVector; }
	const FVector Direction = (Rail->GetEndWorld()-Rail->GetStartWorld()).GetSafeNormal();
	const UMineTransportSubsystem* Network = Rail->GetWorld()->GetSubsystem<UMineTransportSubsystem>();
	const bool bNearStart = Alpha < 0.35f;
	const bool bNearEnd = Alpha > 0.65f;
	if (Network == nullptr || (bNearStart == false && bNearEnd == false)) { return Direction; }
	const FIntPoint Node = bNearStart ? Rail->GetSaveData().Start : Rail->GetSaveData().End;
	const ARailTrackActor* Neighbor = Network->FindNextRail(Rail,Node);
	if (Neighbor == nullptr) { return Direction; }
	const FVector OtherDirection = (Neighbor->GetEndWorld()-Neighbor->GetStartWorld()).GetSafeNormal();
	const FVector JointDirection = (Direction+OtherDirection).GetSafeNormal();
	const float Blend = FMath::SmoothStep(0.f,1.f,bNearStart ? Alpha/0.35f : (1.f-Alpha)/0.35f);
	return FMath::Lerp(JointDirection,Direction,Blend).GetSafeNormal();
}

FVector AMineCartActor::CalculateRailPosition(const ARailTrackActor* Rail, float Alpha)
{
	if (Rail == nullptr) { return FVector::ZeroVector; }
	const FVector Direction = CalculateRailDirection(Rail,Alpha);
	return FMath::Lerp(Rail->GetStartWorld(),Rail->GetEndWorld(),FMath::Clamp(Alpha,0.f,1.f)) + FVector(-Direction.Z,0.f,Direction.X) * 5.5f;
}
