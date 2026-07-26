// LINK

#include "ProjectMK/UI/MKMenuBase.h"

#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "ProjectMK/UI/MKMenuContentsBase.h"

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
	SetVisible(true);
	return true;
}

void UMKMenuBase::CloseMenu()
{
	CurrentMenuContentsType = EMenuContentsType::None;
	SetVisible(false);
}

bool UMKMenuBase::IsMenuOpen() const
{
	return IsVisible();
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
}
