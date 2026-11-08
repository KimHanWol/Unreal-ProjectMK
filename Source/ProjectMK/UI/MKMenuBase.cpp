// LINK

#include "ProjectMK/UI/MKMenuBase.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Overlay.h"
#include "Components/OverlaySlot.h"
#include "Components/TextBlock.h"
#include "Components/Border.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/SizeBox.h"
#include "Components/ButtonSlot.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/UI/MKMenuContentsBase.h"
#include "ProjectMK/UI/HUDWidget.h"
#include "ProjectMK/UI/CraftingMenuContents.h"
#include "ProjectMK/UI/ShopMenuContents.h"

UMKMenuBase::UMKMenuBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RuntimeMenuContentsClasses.Add(UShopMenuContents::StaticClass());
	RuntimeMenuContentsClasses.Add(UCraftingMenuContents::StaticClass());
}

void UMKMenuBase::NativeConstruct()
{
	Super::NativeConstruct();
	UOverlay* Shell = ::IsValid(MenuContentsCanvasPanel) ? Cast<UOverlay>(MenuContentsCanvasPanel->GetParent()) : nullptr;
	if (::IsValid(Shell) && ::IsValid(MenuTitle) == false)
	{
		if (UCanvasPanel* Canvas = Cast<UCanvasPanel>(WidgetTree->RootWidget))
		{
			Canvas->SetVisibility(ESlateVisibility::SelfHitTestInvisible);
			UBorder* Backdrop = BuildModalBackdrop();
			Backdrop->SetVisibility(ESlateVisibility::HitTestInvisible);
			UCanvasPanelSlot* BackdropLayout = Canvas->AddChildToCanvas(Backdrop);
			BackdropLayout->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
			BackdropLayout->SetOffsets(FMargin(0.f));
			BackdropLayout->SetZOrder(-1);
		}
		UTextBlock* TitleText = nullptr;
		UBorder* Header = BuildSectionHeader(FText::GetEmpty(), NAME_None, &TitleText);
		MenuTitle = TitleText;
		UHorizontalBox* HeaderRow = WidgetTree->ConstructWidget<UHorizontalBox>();
		USizeBox* HeaderSize = WidgetTree->ConstructWidget<USizeBox>();
		HeaderSize->SetHeightOverride(48.f);
		HeaderSize->SetContent(Header);
		UHorizontalBoxSlot* TitleLayout = HeaderRow->AddChildToHorizontalBox(HeaderSize);
		TitleLayout->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
		TitleLayout->SetVerticalAlignment(VAlign_Center);
		TitleLayout->SetPadding(FMargin(0.f, 0.f, 12.f, 0.f));
		ExitButton->RemoveFromParent();
		UWidget* CloseArt = ExitButton->GetContent();
		ExitButton->SetContent(nullptr);
		USizeBox* CloseArtSize = WidgetTree->ConstructWidget<USizeBox>();
		CloseArtSize->SetWidthOverride(32.f);
		CloseArtSize->SetHeightOverride(32.f);
		CloseArtSize->SetContent(CloseArt);
		ExitButton->SetContent(CloseArtSize);
		CastChecked<UButtonSlot>(CloseArtSize->Slot)->SetHorizontalAlignment(HAlign_Center);
		CastChecked<UButtonSlot>(CloseArtSize->Slot)->SetVerticalAlignment(VAlign_Center);
		USizeBox* CloseSize = WidgetTree->ConstructWidget<USizeBox>();
		CloseSize->SetWidthOverride(48.f);
		CloseSize->SetHeightOverride(48.f);
		CloseSize->SetContent(ExitButton);
		HeaderRow->AddChildToHorizontalBox(CloseSize)->SetVerticalAlignment(VAlign_Center);
		UOverlaySlot* HeaderLayout = Shell->AddChildToOverlay(HeaderRow);
		HeaderLayout->SetHorizontalAlignment(HAlign_Fill);
		HeaderLayout->SetVerticalAlignment(VAlign_Top);
		HeaderLayout->SetPadding(FMargin(32.f, 20.f, 24.f, 0.f));
		CastChecked<UOverlaySlot>(MenuContentsCanvasPanel->Slot)->SetPadding(FMargin(24.f, 80.f, 24.f, 24.f));
		ApplyVisualStyle();
	}
}

bool UMKMenuBase::NativeOnDrop(const FGeometry& Geometry, const FDragDropEvent& Event, UDragDropOperation* Operation)
{
	if (UHUDWidget* HUD = GetHUDWidget()) { return HUD->NativeOnDrop(Geometry, Event, Operation); }
	return Super::NativeOnDrop(Geometry, Event, Operation);
}

void UMKMenuBase::BindEvents()
{
	Super::BindEvents();

	if (::IsValid(ExitButton))
	{
		ExitButton->OnClicked.AddDynamic(this, &UMKMenuBase::OnExitButtonClicked);
	}
}

void UMKMenuBase::UnbindEvents()
{
	Super::UnbindEvents();
	if (AMKCharacter* Character = GetLocalPlayerCharacter())
	{
		Character->SetMenuInputBlocked(false);
	}

	if (::IsValid(ExitButton))
	{
		ExitButton->OnClicked.RemoveAll(this);
	}
}

void UMKMenuBase::UpdateWidget()
{
	CacheMenuContents();
	SetVisible(false);
}

bool UMKMenuBase::OpenMenu(EMenuContentsType InMenuContentsType)
{
	if (InMenuContentsType == EMenuContentsType::None)
	{
		return false;
	}

	if (UpdateMenuContents(InMenuContentsType) == false)
	{
		return false;
	}

	CurrentMenuContentsType = InMenuContentsType;
	if (UHUDWidget* HUD = GetHUDWidget())
	{
		HUD->SetHoveredSlot(nullptr);
	}
	if (AMKCharacter* Character = GetLocalPlayerCharacter())
	{
		Character->SetMenuInputBlocked(true);
	}
	SetVisible(true);
	SetVisibility(ESlateVisibility::SelfHitTestInvisible);
	USoundManager::TryPlaySFX(this, ESFXType::UI_MenuOpen);
	return true;
}

void UMKMenuBase::CloseMenu()
{
	if (IsMenuOpen())
	{
		USoundManager::TryPlaySFX(this, ESFXType::UI_MenuClose);
	}

	CurrentMenuContentsType = EMenuContentsType::None;
	if (::IsValid(CurrentMenuContents))
	{
		CurrentMenuContents->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (UHUDWidget* HUD = GetHUDWidget())
	{
		HUD->SetHoveredSlot(nullptr);
	}
	if (AMKCharacter* Character = GetLocalPlayerCharacter())
	{
		Character->SetMenuInputBlocked(false);
	}
	SetVisible(false);
}

bool UMKMenuBase::IsMenuOpen() const
{
	return IsVisible();
}

bool UMKMenuBase::IsMenuOpen(EMenuContentsType InMenuContentsType) const
{
	return IsMenuOpen() && CurrentMenuContentsType == InMenuContentsType;
}

void UMKMenuBase::OnExitButtonClicked()
{
	CloseMenu();
}

void UMKMenuBase::CacheMenuContents()
{
	MenuContentsMap.Empty();
	CurrentMenuContents = nullptr;

	if (::IsValid(MenuContentsCanvasPanel) == false)
	{
		return;
	}

	const int32 ChildrenCount = MenuContentsCanvasPanel->GetChildrenCount();
	for (int32 ChildIndex = 0; ChildIndex < ChildrenCount; ChildIndex++)
	{
		UMKMenuContentsBase* MenuContents = Cast<UMKMenuContentsBase>(MenuContentsCanvasPanel->GetChildAt(ChildIndex));
		if (::IsValid(MenuContents) == false)
		{
			continue;
		}

		MenuContentsMap.FindOrAdd(MenuContents->GetMenuContentsType()) = MenuContents;
		MenuContents->SetVisibility(ESlateVisibility::Collapsed);
	}

	for (const TSubclassOf<UMKMenuContentsBase>& RuntimeMenuContentsClass : RuntimeMenuContentsClasses)
	{
		const UMKMenuContentsBase* MenuContentsCDO = RuntimeMenuContentsClass.GetDefaultObject();
		if (::IsValid(MenuContentsCDO) == false || MenuContentsMap.Contains(MenuContentsCDO->GetMenuContentsType()))
		{
			continue;
		}

		UMKMenuContentsBase* MenuContents = CreateWidget<UMKMenuContentsBase>(this, RuntimeMenuContentsClass);
		UCanvasPanelSlot* MenuContentsSlot = ::IsValid(MenuContents) ? MenuContentsCanvasPanel->AddChildToCanvas(MenuContents) : nullptr;
		if (::IsValid(MenuContentsSlot) == false)
		{
			continue;
		}

		MenuContentsSlot->SetAnchors(FAnchors(0.f, 0.f, 1.f, 1.f));
		MenuContentsSlot->SetOffsets(FMargin(0.f));
		MenuContentsMap.FindOrAdd(MenuContents->GetMenuContentsType()) = MenuContents;
		MenuContents->SetVisibility(ESlateVisibility::Collapsed);
	}
}
bool UMKMenuBase::UpdateMenuContents(EMenuContentsType InMenuContentsType)
{
	if (::IsValid(MenuContentsCanvasPanel) == false)
	{
		return false;
	}

	if (MenuContentsMap.IsEmpty())
	{
		CacheMenuContents();
	}

	UMKMenuContentsBase* TargetMenuContents = nullptr;
	if (TObjectPtr<UMKMenuContentsBase>* FoundMenuContents = MenuContentsMap.Find(InMenuContentsType))
	{
		TargetMenuContents = FoundMenuContents->Get();
	}

	if (::IsValid(TargetMenuContents) == false)
	{
		return false;
	}

	for (const TPair<EMenuContentsType, TObjectPtr<UMKMenuContentsBase>>& MenuContentsPair : MenuContentsMap)
	{
		if (::IsValid(MenuContentsPair.Value))
		{
			MenuContentsPair.Value->SetVisibility(MenuContentsPair.Key == InMenuContentsType ? ESlateVisibility::Visible : ESlateVisibility::Collapsed);
		}
	}

	CurrentMenuContents = TargetMenuContents;
	if (::IsValid(MenuTitle))
	{
		const FText Title = InMenuContentsType == EMenuContentsType::Crafting ? NSLOCTEXT("CraftingMenu", "Title", "제작 작업대")
			: (InMenuContentsType == EMenuContentsType::Shop ? NSLOCTEXT("Shop", "Title", "광산 보급소") : NSLOCTEXT("SkillTreeMenu", "Title", "기술 설계도"));
		MenuTitle->SetText(Title);
	}
	return true;
}
