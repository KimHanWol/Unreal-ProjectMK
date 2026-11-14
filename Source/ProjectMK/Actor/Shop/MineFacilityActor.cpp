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

bool AMineFacilityActor::OnInteract(AActor* Interactor)
{
	TArray<UUserWidget*> Widgets;
	UWidgetBlueprintLibrary::GetAllWidgetsOfClass(this, Widgets, UHUDWidget::StaticClass(), false);
	if (Widgets.IsEmpty()) { return false; }
	UHUDWidget* HUD = CastChecked<UHUDWidget>(Widgets[0]);
	if (FacilityKind == EMineFacilityKind::RailFactory && GetWorld()->GetSubsystem<UTownProgressSubsystem>()->IsRailFactoryUnlocked()) { return HUD->TryOpenShop(TEXT("RailFactory")); }
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
	InteractionBox->SetBoxExtent(FVector(Kind == EMineFacilityKind::Elevator ? 28.f : (Kind == EMineFacilityKind::RailFactory ? 26.f : 20.f), 36.f, 26.f));
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
	case EMineFacilityKind::HouseA: return NSLOCTEXT("Town", "MinerResident", "광부의 집 · 광부");
	default: return NSLOCTEXT("Town", "MechanicResident", "정비공의 집 · 정비공");
	}
}
FName AMineFacilityActor::GetArtKey() const
{
	static const FName Keys[] = { TEXT("Elevator"), TEXT("RailFactory"), TEXT("HouseA"), TEXT("HouseB") };
	return Keys[static_cast<int32>(FacilityKind)];
}
