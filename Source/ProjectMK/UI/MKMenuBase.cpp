// LINK

#include "ProjectMK/UI/MKMenuBase.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/SoundManager.h"
#include "ProjectMK/UI/MKMenuContentsBase.h"
#include "ProjectMK/UI/CraftingMenuContents.h"
#include "ProjectMK/UI/ShopMenuContents.h"

UMKMenuBase::UMKMenuBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	RuntimeMenuContentsClasses.Add(UShopMenuContents::StaticClass());
	RuntimeMenuContentsClasses.Add(UCraftingMenuContents::StaticClass());
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
	return true;
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
