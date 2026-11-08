#include "ProjectMK/UI/MenuDisplayCardWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/Image.h"
#include "Components/ScaleBox.h"
#include "Components/SizeBox.h"
#include "Brushes/SlateNoResource.h"
#include "PaperSprite.h"
#include "ProjectMK/Actor/Character/MKCharacter.h"
#include "ProjectMK/Core/Manager/DataManager.h"
#include "ProjectMK/Data/DataAsset/VisualStyleDataAsset.h"

TSharedRef<SWidget> UMenuDisplayCardWidget::RebuildWidget()
{
	if (::IsValid(WidgetTree) && WidgetTree->RootWidget == nullptr)
	{
		USizeBox* Size = WidgetTree->ConstructWidget<USizeBox>();
		Size->SetHeightOverride(180.f);
		WidgetTree->RootWidget = Size;
		Stage = WidgetTree->ConstructWidget<UBorder>();
		Stage->SetPadding(FMargin(20.f));
		Stage->SetHorizontalAlignment(HAlign_Center);
		Stage->SetVerticalAlignment(VAlign_Center);
		Size->SetContent(Stage);
		Portrait = WidgetTree->ConstructWidget<UImage>();
		USizeBox* ImageSize = WidgetTree->ConstructWidget<USizeBox>();
		ImageSize->SetWidthOverride(128.f);
		ImageSize->SetHeightOverride(128.f);
		UScaleBox* ImageScale = WidgetTree->ConstructWidget<UScaleBox>();
		ImageScale->SetStretch(EStretch::ScaleToFit);
		ImageScale->SetContent(Portrait);
		ImageSize->SetContent(ImageScale);
		Stage->SetContent(ImageSize);
	}
	return Super::RebuildWidget();
}

void UMenuDisplayCardWidget::SetDisplayRole(FName InRole)
{
	DisplayRole = InRole;
	UpdateWidget();
}

void UMenuDisplayCardWidget::UpdateWidget()
{
	if (::IsValid(Portrait) == false) { return; }
	Stage->SetBrush(FSlateNoResource());
	if (DisplayRole == TEXT("Player"))
	{
		const AMKCharacter* Character = GetLocalPlayerCharacter();
		const UPaperSprite* Sprite = ::IsValid(Character) ? Character->GetPortraitSprite() : nullptr;
		if (::IsValid(Sprite))
		{
			FSlateBrush Brush;
			Brush.SetResourceObject(const_cast<UPaperSprite*>(Sprite));
			Brush.ImageSize = Sprite->GetSourceSize();
			Portrait->SetBrush(Brush);
		}
	}
	else
	{
		const UDataManager* Data = UDataManager::Get(this);
		const UVisualStyleDataAsset* Style = ::IsValid(Data) ? Data->GetVisualStyleDataAsset() : nullptr;
		if (::IsValid(Style))
		{
			if (const FSlateBrush* Brush = Style->Brushes.Find(DisplayRole)) { Portrait->SetBrush(*Brush); }
		}
	}
}
