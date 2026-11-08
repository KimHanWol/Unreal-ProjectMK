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
		UTextBlock* TitleText = nullptr;
		UBorder* Header = BuildSectionHeader(FText::GetEmpty(), NAME_None, &TitleText);
		MenuTitle = TitleText;
		UOverlaySlot* HeaderLayout = Shell->AddChildToOverlay(Header);
		HeaderLayout->SetHorizontalAlignment(HAlign_Fill);
		HeaderLayout->SetVerticalAlignment(VAlign_Top);
		HeaderLayout->SetPadding(FMargin(32.f, 20.f, 88.f, 0.f));
		CastChecked<UOverlaySlot>(MenuContentsCanvasPanel->Slot)->SetPadding(FMargin(24.f, 80.f, 24.f, 24.f));
		ApplyVisualStyle();
	}
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
