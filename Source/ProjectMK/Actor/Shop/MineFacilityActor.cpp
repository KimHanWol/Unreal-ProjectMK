#include "ProjectMK/Actor/Shop/MineFacilityActor.h"

#include "Blueprint/WidgetBlueprintLibrary.h"
#include "Components/BoxComponent.h"
#include "PaperSprite.h"
#include "PaperSpriteComponent.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/TownProgressSubsystem.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
#include "ProjectMK/UI/HUDWidget.h"

bool AMineFacilityActor::CanInteract(AActor* Interactor)
{
	if (Interactor == nullptr || ShopSprite->GetSprite() == nullptr) { return false; }
	const FVector Local = Interactor->GetActorLocation() - GetActorLocation();
	const FVector2D Size = ShopSprite->GetSprite()->GetSourceSize() / ShopSprite->GetSprite()->GetPixelsPerUnrealUnit();
	return FMath::Abs(Local.X) <= Size.X * 0.5f && FMath::Abs(Local.Y) <= 36.f && Local.Z >= -12.f && Local.Z <= Size.Y - 12.f;
}

bool AMineFacilityActor::OnInteract(AActor* Interactor)
{
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UHUDWidget::StaticClass(), false);
	if (Widgets.IsEmpty()) { return false; }
	UHUDWidget* HUD = CastChecked<UHUDWidget>(Widgets[0]);
	if (FacilityKind == EMineFacilityKind::RailFactory && GetWorld()->GetSubsystem<UTownProgressSubsystem>()->IsRailFactoryUnlocked()) { return HUD->TryOpenShop(TEXT("RailFactory")); }
	GetWorld()->GetSubsystem<UTownProgressSubsystem>()->BeginResidentConversation(Cast<AMKCharacter>(Interactor), this);
	return HUD->OpenFacility(this);
}

void AMineFacilityActor::InitializeFacility(EMineFacilityKind Kind, int32 Depth, const FVector& FloorLocation)
{
	FacilityKind = Kind; StationDepth = Depth;
	const auto* Style = UDataManager::Get(this)->GetVisualStyleDataAsset();
	UPaperSprite* Sprite = Style != nullptr ? Style->TownSprites.FindRef(GetArtKey()) : nullptr;
	const float HalfHeight = Sprite != nullptr ? Sprite->GetSourceSize().Y / Sprite->GetPixelsPerUnrealUnit() * 0.5f : 24.f;
	SetActorLocation(FloorLocation + FVector(0.f, 0.f, 12.f));
	ShopSprite->SetSprite(Sprite);
	ShopSprite->SetRelativeLocation(FVector(0.f, -14.f, HalfHeight - 12.f));
	const float HalfWidth = Sprite != nullptr ? Sprite->GetSourceSize().X / Sprite->GetPixelsPerUnrealUnit() * 0.5f : 24.f;
	InteractionBox->SetBoxExtent(FVector(FMath::Max(16.f, HalfWidth), 36.f, FMath::Max(24.f, HalfHeight * 2.f)));
	Tags.Add(TEXT("MineFacility"));
}

EMineFacilityKind AMineFacilityActor::GetFacilityKind() const { return FacilityKind; }
int32 AMineFacilityActor::GetStationDepth() const { return StationDepth; }
FText AMineFacilityActor::GetFacilityName() const
{
	switch (FacilityKind)
	{
	case EMineFacilityKind::Elevator: return NSLOCTEXT("Town", "Elevator", "광산 엘리베이터");
	case EMineFacilityKind::RailFactory: return NSLOCTEXT("Town", "RailFactory", "철도 공장");
	case EMineFacilityKind::QuestBoard: return NSLOCTEXT("Town", "QuestBoard", "퇴치 의뢰 게시판");
	case EMineFacilityKind::HouseA: return NSLOCTEXT("Town", "MinerResident", "광부의 집 · 광부");
	default: return NSLOCTEXT("Town", "MechanicResident", "정비공의 집 · 정비공");
	}
}
FName AMineFacilityActor::GetArtKey() const
{
	static const FName Keys[] = { TEXT("Elevator"), TEXT("RailFactory"), TEXT("HouseA"), TEXT("HouseB"), TEXT("VillageNotice") };
	return Keys[static_cast<int32>(FacilityKind)];
}
