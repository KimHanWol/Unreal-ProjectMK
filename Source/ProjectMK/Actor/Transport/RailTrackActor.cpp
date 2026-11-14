#include "ProjectMK/Actor/Transport/RailTrackActor.h"

#include "Components/BoxComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/MineTransportSubsystem.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"

ARailTrackActor::ARailTrackActor()
{
	PrimaryActorTick.bCanEverTick = false;
	InteractionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionBox"));
	RootComponent = InteractionBox;
	InteractionBox->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	InteractionBox->SetCollisionResponseToAllChannels(ECR_Ignore);
	InteractionBox->SetCollisionResponseToChannel(ECC_Visibility, ECR_Overlap);
	RailSprite = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("Rail"));
	RailSprite->SetupAttachment(RootComponent);
	StartCap = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("StartCap"));
	StartCap->SetupAttachment(RootComponent);
	EndCap = CreateDefaultSubobject<UPaperSpriteComponent>(TEXT("EndCap"));
	EndCap->SetupAttachment(RootComponent);
	for (auto* Render : { RailSprite.Get(), StartCap.Get(), EndCap.Get() })
	{
		Render->SetCollisionEnabled(ECollisionEnabled::NoCollision);
		Render->SetTranslucentSortPriority(-4);
		Render->SetRelativeLocation(FVector(0.f, -10.f, 0.f));
	}
}

bool ARailTrackActor::CanInteract(AActor* Interactor)
{
	return Interactor != nullptr && FVector::DistSquared(Interactor->GetActorLocation(), FMath::ClosestPointOnSegment(Interactor->GetActorLocation(), GetStartWorld(), GetEndWorld())) < FMath::Square(28.f);
}

FName ARailTrackActor::GetRecoveryItemKey() const { return TEXT("Rail"); }

FText ARailTrackActor::GetRecoveryBlockedReason(AActor* Interactor) const
{
	if (GetWorld()->GetSubsystem<UMineTransportSubsystem>()->HasCartOnLine(this)) { return NSLOCTEXT("Rail", "RemoveCartFirst", "열차를 먼저 회수하세요"); }
	return IInteractable::GetRecoveryBlockedReason(Interactor);
}

void ARailTrackActor::EndPlay(const EEndPlayReason::Type Reason)
{
	if (Reason == EEndPlayReason::Destroyed) { GetWorld()->GetSubsystem<UMineTransportSubsystem>()->NotifyRailRemoved(this); }
	Super::EndPlay(Reason);
}

void ARailTrackActor::InitializeRail(const FRailSaveData& Data)
{
	RailData = Data;
	const FVector A = GetStartWorld(), B = GetEndWorld();
	const FVector Direction = B - A;
	SetActorLocation((A + B) * 0.5f);
	SetActorRotation(Direction.Rotation());
	InteractionBox->SetBoxExtent(FVector(Direction.Size() * 0.5f, 14.f, 5.f));
	const auto* Style = UDataManager::Get(this)->GetVisualStyleDataAsset();
	UPaperSprite* Sprite = Style != nullptr ? Style->TownSprites.FindRef(TEXT("Rail")) : nullptr;
	RailSprite->SetSprite(Sprite);
	if (Sprite != nullptr) { RailSprite->SetRelativeScale3D(FVector(Direction.Size() / (Sprite->GetSourceSize().X / Sprite->GetPixelsPerUnrealUnit()), 1.f, 1.f)); }
	for (auto* Cap : { StartCap.Get(), EndCap.Get() }) { Cap->SetSprite(Style != nullptr ? Style->TownSprites.FindRef(TEXT("RailEnd")) : nullptr); }
	StartCap->SetRelativeLocation(FVector(-Direction.Size() * 0.5f, -9.5f, 1.f));
	EndCap->SetRelativeLocation(FVector(Direction.Size() * 0.5f, -9.5f, 1.f));
}

void ARailTrackActor::UpdateEndCaps(bool bStart, bool bEnd) { StartCap->SetVisibility(bStart); EndCap->SetVisibility(bEnd); }
FVector ARailTrackActor::GetStartWorld() const { return FVector((RailData.Start.X + 0.5f) * 16.f, -20.f, -RailData.Start.Y * 16.f + 0.5f); }
FVector ARailTrackActor::GetEndWorld() const
{
	const FVector End((RailData.End.X + 0.5f) * 16.f, -20.f, -RailData.End.Y * 16.f + 0.5f);
	return RailData.bEndDisconnected ? End - (End - GetStartWorld()).GetSafeNormal() * 5.f : End;
}
const FRailSaveData& ARailTrackActor::GetSaveData() const { return RailData; }
