#include "ProjectMK/UI/RecipeInfoWidget.h"

#include "Blueprint/WidgetTree.h"
#include "Brushes/SlateNoResource.h"
#include "Components/Border.h"
#include "ProjectMK/UI/HUDWidget.h"

TSharedRef<SWidget> URecipeInfoWidget::RebuildWidget()
{
	if (WidgetTree->RootWidget == nullptr)
	{
		ContentFrame = WidgetTree->ConstructWidget<UBorder>();
		ContentFrame->SetBrush(FSlateNoResource());
		ContentFrame->SetPadding(FMargin(0.f));
		WidgetTree->RootWidget = ContentFrame;
	}
	return Super::RebuildWidget();
}

void URecipeInfoWidget::NativeDestruct()
{
	if (UHUDWidget* HUD = GetHUDWidget()) { HUD->ClearTooltipSource(this); }
	Super::NativeDestruct();
}

void URecipeInfoWidget::SetInfoContent(UWidget* Content)
{
	TakeWidget();
	ContentFrame->SetContent(Content);
}

void URecipeInfoWidget::SetRecipe(FName Key, bool bBlueprint)
{
	RecipeKey = Key;
	bIsBlueprint = bBlueprint;
	bIsItem = false;
}

void URecipeInfoWidget::SetItem(FName Key)
{
	RecipeKey = Key;
	bIsItem = true;
}

void URecipeInfoWidget::NativeOnMouseEnter(const FGeometry& Geometry, const FPointerEvent& Event)
{
	Super::NativeOnMouseEnter(Geometry, Event);
	if (UHUDWidget* HUD = GetHUDWidget())
	{
		if (bIsItem) { HUD->ShowItemTooltip(this, RecipeKey); }
		else { HUD->ShowRecipeTooltip(this, RecipeKey, bIsBlueprint); }
	}
}

void URecipeInfoWidget::NativeOnMouseLeave(const FPointerEvent& Event)
{
	Super::NativeOnMouseLeave(Event);
	if (UHUDWidget* HUD = GetHUDWidget()) { HUD->ClearTooltipSource(this); }
}
