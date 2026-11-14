#include "ProjectMK/UI/FacilityMenuContents.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "Components/SizeBox.h"
#include "Components/ScaleBox.h"
#include "PaperSprite.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "ProjectMK/Actor/Shop/MineFacilityActor.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Component/InventoryComponent.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Core/Subsystem/TownProgressSubsystem.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"
#include "ProjectMK/UI/HUDWidget.h"

UFacilityMenuContents::UFacilityMenuContents(const FObjectInitializer& Initializer) : Super(Initializer) { MenuContentsType = EMenuContentsType::Facility; }

TSharedRef<SWidget> UFacilityMenuContents::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr) { BuildDefaultWidgetTree(); }
	return Super::RebuildWidget();
}

void UFacilityMenuContents::NativeTick(const FGeometry& Geometry, float DeltaSeconds)
{
	Super::NativeTick(Geometry, DeltaSeconds);
	if (Facility.IsValid() && Facility->CanInteract(GetLocalPlayerCharacter()) == false) { GetHUDWidget()->CloseFacility(); }
}

void UFacilityMenuContents::BindEvents()
{
	Super::BindEvents();
	GetWorld()->GetSubsystem<UTownProgressSubsystem>()->OnTownProgressChanged.AddUObject(this, &UFacilityMenuContents::UpdateWidget);
	if (UInventoryComponent* Inventory = GetLocalInventoryComponent()) { Inventory->OnInventoryChangedDelegate.AddUObject(this, &UFacilityMenuContents::UpdateWidget); }
	Actions[0]->OnClicked.AddUniqueDynamic(this, &UFacilityMenuContents::OnPrimaryClicked);
	Actions[1]->OnClicked.AddUniqueDynamic(this, &UFacilityMenuContents::OnSecondaryClicked);
	Actions[2]->OnClicked.AddUniqueDynamic(this, &UFacilityMenuContents::OnTertiaryClicked);
}

void UFacilityMenuContents::UnbindEvents()
{
	GetWorld()->GetSubsystem<UTownProgressSubsystem>()->OnTownProgressChanged.RemoveAll(this);
	if (UInventoryComponent* Inventory = GetLocalInventoryComponent()) { Inventory->OnInventoryChangedDelegate.RemoveAll(this); }
	for (UButton* Button : Actions) { Button->OnClicked.RemoveAll(this); }
	Super::UnbindEvents();
}

void UFacilityMenuContents::UpdateWidget()
{
	if (Facility.IsValid() == false || Dialogue == nullptr) { return; }
	UTownProgressSubsystem* Town = GetWorld()->GetSubsystem<UTownProgressSubsystem>();
	Dialogue->SetText(Town->GetDialogue(Facility.Get(), GetLocalPlayerCharacter()));
	const auto* Art = UDataManager::Get(this)->GetVisualStyleDataAsset();
	UPaperSprite* Sprite = Art != nullptr ? Art->TownSprites.FindRef(Facility->GetArtKey()) : nullptr;
	FSlateBrush Brush; Brush.SetResourceObject(Sprite); Brush.ImageSize = Sprite != nullptr ? Sprite->GetSourceSize() : FVector2D(64.f, 96.f); FacilityImage->SetBrush(Brush);
	for (USizeBox* Frame : ActionFrames) { Frame->SetVisibility(ESlateVisibility::Collapsed); }
	if (Facility->GetFacilityKind() == EMineFacilityKind::Elevator)
	{
		Destinations = { 0 };
		for (int32 Depth : UDataManager::Get(this)->GetGameSettingDataAsset()->TownFacilities.ElevatorDepths) { if (Destinations.Num() < Actions.Num()) { Destinations.AddUnique(Depth); } }
		for (int32 Index = 0; Index < Destinations.Num(); ++Index)
		{
			const int32 Depth = Destinations[Index];
			Labels[Index]->SetText(Depth == 0 ? NSLOCTEXT("Town", "SurfaceStation", "지상으로 이동") : FText::Format(NSLOCTEXT("Town", "DepthStation", "{0}m 정거장"), FText::AsNumber(Depth)));
			ActionFrames[Index]->SetVisibility(ESlateVisibility::Visible);
			Actions[Index]->SetIsEnabled(Town->CanTravelToDepth(Depth) && Depth != Facility->GetStationDepth());
		}
	}
	else if (Facility->GetFacilityKind() == EMineFacilityKind::HouseA || Facility->GetFacilityKind() == EMineFacilityKind::HouseB)
	{
		ActionFrames[0]->SetVisibility(ESlateVisibility::Visible);
		Actions[0]->SetIsEnabled(Town->CanUseQuestAction(Facility.Get(), GetLocalPlayerCharacter()));
		Labels[0]->SetText(Town->GetQuestActionLabel(Facility.Get()));
	}
}

void UFacilityMenuContents::SetFacility(AMineFacilityActor* Actor) { Facility = Actor; UpdateWidget(); }

void UFacilityMenuContents::BuildDefaultWidgetTree()
{
	UVerticalBox* Column = WidgetTree->ConstructWidget<UVerticalBox>();
	WidgetTree->RootWidget = Column;
	USizeBox* Preview = WidgetTree->ConstructWidget<USizeBox>();
	Preview->SetWidthOverride(256.f); Preview->SetHeightOverride(192.f);
	Column->AddChildToVerticalBox(Preview)->SetHorizontalAlignment(HAlign_Center);
	FacilityImage = WidgetTree->ConstructWidget<UImage>();
	UScaleBox* Scale = WidgetTree->ConstructWidget<UScaleBox>();
	Scale->SetStretch(EStretch::ScaleToFit); Scale->SetContent(FacilityImage); Preview->SetContent(Scale);
	Dialogue = WidgetTree->ConstructWidget<UTextBlock>(UTextBlock::StaticClass(), TEXT("FacilityDialogue"));
	ApplyTextRole(Dialogue, EMKTextRole::Body, true);
	Dialogue->SetJustification(ETextJustify::Center);
	UVerticalBoxSlot* DialogueSlot = Column->AddChildToVerticalBox(Dialogue);
	DialogueSlot->SetPadding(FMargin(32.f, 24.f));
	DialogueSlot->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
	DialogueSlot->SetVerticalAlignment(VAlign_Center);
	for (int32 Index = 0; Index < 3; ++Index)
	{
		UButton* Button = nullptr; UTextBlock* Label = nullptr;
		USizeBox* Action = BuildActionButton(Button, Label, FText::GetEmpty(), 260.f, 44.f);
		UVerticalBoxSlot* ActionSlot = Column->AddChildToVerticalBox(Action);
		ActionSlot->SetPadding(FMargin(24.f, 6.f)); ActionSlot->SetHorizontalAlignment(HAlign_Center);
		Actions.Add(Button); Labels.Add(Label); ActionFrames.Add(Action);
	}
}

void UFacilityMenuContents::ApplyAction(int32 Index)
{
	if (Facility.IsValid() == false) { return; }
	UTownProgressSubsystem* Town = GetWorld()->GetSubsystem<UTownProgressSubsystem>();
	if (Facility->GetFacilityKind() == EMineFacilityKind::Elevator)
	{
		if (Destinations.IsValidIndex(Index) && Town->TryTravel(GetLocalPlayerCharacter(), Facility.Get(), Destinations[Index])) { GetHUDWidget()->CloseFacility(); }
	}
	else { Town->TryQuestAction(GetLocalPlayerCharacter(), Facility.Get()); }
	UpdateWidget();
}

void UFacilityMenuContents::OnPrimaryClicked() { ApplyAction(0); }
void UFacilityMenuContents::OnSecondaryClicked() { ApplyAction(1); }
void UFacilityMenuContents::OnTertiaryClicked() { ApplyAction(2); }
